#!/usr/bin/perl

# Tests for ngx_stream_extra_variables_module.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;
use Test::Nginx::Stream qw/ stream /;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()
	->has(qw/stream stream_return ngx_stream_extra_variables_module/)
	->plan(49);

my $timing = $ENV{TEST_NGINX_UPSTREAM_TIMING} // 1;
my $status = $ENV{TEST_NGINX_UPSTREAM_STATUS} // 1;
my $config = <<'EOF';

%%TEST_GLOBALS%%

daemon off;

events {
}

stream {
    %%TEST_GLOBALS_STREAM%%

    log_format metrics
        'dollar=$dollar|sec=$sec|hex=$hextime|conn=$connection_established_msec|created=$session_created_msec|upper=$hostname_uppercase|lower=$hostname_lowercase|rfc=$time_rfc1123|addr=$upstream_last_addr|tries=$upstream_tries|status=$upstream_last_status|start=$upstream_start_msec|last_start=$upstream_last_start_msec|connected=$upstream_connected_msec|last_connected=$upstream_last_connected_msec|first=$upstream_first_byte_msec|last_first=$upstream_last_first_byte_msec|end=$upstream_end_msec|last_end=$upstream_last_end_msec|connect_time=$upstream_last_connect_time|first_time=$upstream_last_first_byte_time|session_time=$upstream_last_session_time|last_received=$upstream_last_bytes_received|last_sent=$upstream_last_bytes_sent|total_received=$upstream_total_bytes_received|total_sent=$upstream_total_bytes_sent';

    upstream backend {
        server 127.0.0.1:8081;
    }

    upstream retry {
        server 127.0.0.1:8999;
        server 127.0.0.1:8081 backup;
    }

    server {
        listen      127.0.0.1:8080;
        access_log  %%TESTDIR%%/metrics.log metrics;
        proxy_pass  backend;
    }

    server {
        listen  127.0.0.1:8081;
        return  backend;
    }

    server {
        listen      127.0.0.1:8082;
        access_log  %%TESTDIR%%/retry.log metrics;
        proxy_next_upstream on;
        proxy_next_upstream_tries 2;
        proxy_pass  retry;
    }

    server {
        listen      127.0.0.1:8083;
        access_log  %%TESTDIR%%/direct.log metrics;
        return      direct;
    }
}

EOF

$config =~ s/\$upstream_last_status/-/g unless $status;
$config =~ s/\$upstream_(?:last_)?(?:start|connected|first_byte|end)_msec/-/g
    unless $timing;
$t->write_file_expand('nginx.conf', $config);
$t->run();

###############################################################################

is(response(8080), 'backend', 'single upstream response');
is(response(8082), 'backend', 'retry reaches backup upstream');
is(response(8083), 'direct', 'direct stream response');

my %m = fields($t->read_file('metrics.log'));

is($m{dollar}, '$', 'literal dollar variable');
like($m{sec}, qr/^\d+$/, 'seconds variable');
like($m{hex}, qr/^[0-9a-f]+$/, 'hex time variable');
cmp_ok(abs(hex($m{hex}) - $m{sec}), '<=', 1,
	'hex time corresponds to seconds');
like($m{conn}, qr/^\d+\.\d{3}$/, 'connection timestamp');
like($m{created}, qr/^\d+\.\d{3}$/, 'session timestamp');
is($m{upper}, uc($m{lower}), 'uppercase and lowercase host variables');
is($m{lower}, lc($m{lower}), 'lowercase host variable');
like($m{rfc}, qr/^[A-Z][a-z]{2}, \d{2} [A-Z][a-z]{2} \d{4} \d{2}:\d{2}:\d{2} GMT$/,
	'RFC 1123 time variable');
is($m{addr}, '127.0.0.1:' . port(8081), 'last upstream address');
is($m{tries}, '1', 'upstream attempt count');
SKIP: {
	skip 'requires extended upstream status', 1 unless $status;
	is($m{status}, '200', 'last upstream status');
}
SKIP: {
	skip 'requires upstream timing core patch', 11 unless $timing;
	like($m{start}, qr/^\d+\.\d{3}$/, 'upstream start timestamp');
	is($m{start}, $m{last_start}, 'last start timestamp');
	like($m{connected}, qr/^\d+\.\d{3}$/, 'upstream connected timestamp');
	is($m{connected}, $m{last_connected}, 'last connected timestamp');
	like($m{first}, qr/^\d+\.\d{3}$/, 'upstream first byte timestamp');
	is($m{first}, $m{last_first}, 'last first byte timestamp');
	like($m{end}, qr/^\d+\.\d{3}$/, 'upstream end timestamp');
	is($m{end}, $m{last_end}, 'last end timestamp');
	cmp_ok(msec($m{start}), '<=', msec($m{connected}),
		'upstream timestamps are ordered at connect');
	cmp_ok(msec($m{connected}), '<=', msec($m{first}),
		'upstream timestamps are ordered at first byte');
	cmp_ok(msec($m{first}), '<=', msec($m{end}),
		'upstream timestamps are ordered at end');
}
like($m{connect_time}, qr/^\d+\.\d{3}$/, 'last connect duration');
like($m{first_time}, qr/^\d+\.\d{3}$/, 'last first byte duration');
like($m{session_time}, qr/^\d+\.\d{3}$/, 'last session duration');
like($m{last_received}, qr/^\d+$/, 'last bytes received');
like($m{last_sent}, qr/^\d+$/, 'last bytes sent');
is($m{total_received}, $m{last_received},
	'total received bytes for one upstream');
is($m{total_sent}, $m{last_sent}, 'total sent bytes for one upstream');

my %r = fields($t->read_file('retry.log'));

is($r{addr}, '127.0.0.1:' . port(8081), 'retry last upstream address');
is($r{tries}, '2', 'retry upstream attempt count');
SKIP: {
	skip 'requires extended upstream status', 1 unless $status;
	is($r{status}, '200', 'retry last upstream status');
}
SKIP: {
	skip 'requires upstream timing core patch', 4 unless $timing;
	like($r{start}, qr/^\d+\.\d{3}, \d+\.\d{3}$/,
		'retry start timestamps include both attempts');
	like($r{connected}, qr/^-, \d+\.\d{3}$/,
		'retry connected timestamps record failed attempt');
	like($r{first}, qr/^-, \d+\.\d{3}$/,
		'retry first byte timestamps record failed attempt');
	like($r{end}, qr/^\d+\.\d{3}, \d+\.\d{3}$/,
		'retry end timestamps include both attempts');
}
like($r{connect_time}, qr/^\d+\.\d{3}$/, 'retry last connect duration');
like($r{first_time}, qr/^\d+\.\d{3}$/, 'retry last first byte duration');
like($r{session_time}, qr/^\d+\.\d{3}$/, 'retry last session duration');
cmp_ok($r{total_received}, '>=', $r{last_received},
	'retry total received bytes');
cmp_ok($r{total_sent}, '>=', $r{last_sent}, 'retry total sent bytes');

my %d = fields($t->read_file('direct.log'));

is($d{addr}, '-', 'no-upstream address is not found');
is($d{tries}, '-', 'no-upstream attempt count is not found');
SKIP: {
	skip 'requires extended upstream status', 1 unless $status;
	is($d{status}, '-', 'no-upstream status is not found');
}
SKIP: {
	skip 'requires upstream timing core patch', 1 unless $timing;
	is($d{start}, '-', 'no-upstream timestamps are not found');
}


###############################################################################

sub response {
	return stream('127.0.0.1:' . port(shift))->read();
}


sub fields {
	my ($line) = @_;
	$line =~ s/\s+$//;

	my %fields;
	for my $field (split /\|/, $line, -1) {
		my ($name, $value) = split /=/, $field, 2;
		$fields{$name} = $value;
	}

	return %fields;
}


sub msec {
	my ($value) = @_;
	my ($sec, $msec) = split /\./, $value;

	return $sec * 1000 + $msec;
}

###############################################################################
