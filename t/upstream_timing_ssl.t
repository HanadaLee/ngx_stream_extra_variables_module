#!/usr/bin/perl

# Stream upstream timing during TLS negotiation.

use warnings;
use strict;
use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }
use Test::Nginx;
use Test::Nginx::Stream qw/ stream /;

plan(skip_all => 'requires upstream timing core patch')
	if defined $ENV{TEST_NGINX_UPSTREAM_TIMING}
		&& !$ENV{TEST_NGINX_UPSTREAM_TIMING};

my $t = Test::Nginx->new()->has(qw/stream stream_ssl stream_return
	ngx_stream_extra_variables_module/)->has_daemon('openssl')->plan(21);
my @fields = qw/start connected first_byte end/;
my $d = $t->testdir();
my $config = <<'EOF';
%%TEST_GLOBALS%%
daemon off;
events {}
stream {
    %%TEST_GLOBALS_STREAM%%
    log_format timing 'FIELDS';
    server {
        listen 127.0.0.1:8080;
        access_log %%TESTDIR%%/tls.log timing;
        proxy_ssl on;
        proxy_pass 127.0.0.1:8443;
    }
    server {
        listen 127.0.0.1:8443 ssl;
        ssl_certificate cert.pem;
        ssl_certificate_key key.pem;
        return backend;
    }
    server {
        listen 127.0.0.1:8081;
        return plaintext;
    }
    server {
        listen 127.0.0.1:8082;
        access_log %%TESTDIR%%/failed.log timing;
        proxy_ssl on;
        proxy_pass 127.0.0.1:8081;
    }
}
EOF
$config =~ s/FIELDS/join('|', map {
	('$upstream_' . $_ . '_msec', '$upstream_last_' . $_ . '_msec')
} @fields)/e;
$t->write_file_expand('nginx.conf', $config);
$t->write_file('openssl.conf', "[req]\ndistinguished_name = dn\n[dn]\n");
system('openssl req -x509 -newkey rsa:2048 -nodes -days 2 '
	. "-config $d/openssl.conf -subj /CN=timing.test -out $d/cert.pem "
	. "-keyout $d/key.pem >>$d/openssl.out 2>&1") == 0 or die "openssl: $?";
$t->run();
is(stream('127.0.0.1:' . port(8080))->read(), 'backend', 'TLS upstream response');
is(stream('127.0.0.1:' . port(8082))->read(), '', 'TLS negotiation failure');
$t->stop();

my @tls = split /\|/, $t->read_file('tls.log');
chomp $tls[-1];
for my $i (0 .. $#fields) {
	like($tls[2 * $i], qr/^\d+\.\d{3}$/, 'TLS ' . $fields[$i]);
	is($tls[2 * $i + 1], $tls[2 * $i], 'TLS last ' . $fields[$i]);
}
for my $i (1 .. $#fields) {
	cmp_ok($tls[2 * $i], '>=', $tls[2 * $i - 2], 'TLS timestamp order');
}
my @failed = split /\|/, $t->read_file('failed.log');
chomp $failed[-1];
for my $i (0 .. $#fields) {
	like($failed[2 * $i], $fields[$i] =~ /^(?:start|end)$/
		? qr/^\d+\.\d{3}$/ : qr/^-$/, 'failed TLS ' . $fields[$i]);
	is($failed[2 * $i + 1], $failed[2 * $i], 'failed TLS last ' . $fields[$i]);
}
