# ngx_stream_extra_variables_module

A collection of extra variables for NGINX stream module. Used to meet logging or other needs.

Some of these variables require the nginx core patch [nginx-ext.patch](https://git.hanada.info/hanada/openresty/-/tree/main/patches) to work. These variables are marked with * in the following text.

## Embedded Variables

### \$dollar
A literal dollar sign.

### \$sec
Current time in seconds.

### \$hextime
Current time in hexadecimal.

### \$connection_established_msec
Connection established timestamp in seconds with millisecond resolution.

### \$session_created_msec
Session created timestamp in seconds with millisecond resolution.

### \$hostname_uppercase
Uppercase host name.

### \$hostname_lowercase
Lowercase host name.

### \$time_rfc1123
Local time in the RFC 1123 format. The time format is consistent with the HTTP `Date` header.

### \$upstream_last_addr
Keeps the IP address and port, or the path to the UNIX-domain socket of the latest upstream server.

### \$upstream_tries
Number of upstream connection attempts.

### \$upstream_last_status *
Keeps status code of the response obtained from the latest upstream server.

### \$upstream_start_msec *
Keeps timestamps of upstream starts; the time is kept in seconds with millisecond resolution. Times of several responses are separated by commas and colons like addresses in the `$upstream_addr` variable.

### \$upstream_last_start_msec *
Keeps timestamp of the latest upstream start; the time is kept in seconds with millisecond resolution.

### \$upstream_connected_msec *
Keeps timestamps of upstream connections established; the time is kept in seconds with millisecond resolution. Times of several responses are separated by commas and colons like addresses in the `$upstream_addr` variable.

### \$upstream_last_connected_msec *
Keeps timestamp of the latest upstream connection established; the time is kept in seconds with millisecond resolution.

### \$upstream_first_byte_msec *
Keeps timestamps of the first byte received from upstream; the time is kept in seconds with millisecond resolution. Times of several responses are separated by commas and colons like addresses in the `$upstream_addr` variable.

### \$upstream_last_first_byte_msec *
Keeps timestamp of the first byte received from the latest upstream server; the time is kept in seconds with millisecond resolution.

### \$upstream_end_msec *
Keeps timestamps of upstream response end or abnormal interruption; the time is kept in seconds with millisecond resolution. Times of several responses are separated by commas and colons like addresses in the `$upstream_addr` variable.

### \$upstream_last_end_msec *
Keeps timestamp of the latest upstream response end or abnormal interruption; the time is kept in seconds with millisecond resolution.

### \$upstream_last_connect_time
Keeps time spent on establishing a connection with the upstream server; the time is kept in seconds with millisecond resolution. In case of SSL, includes time spent on handshake.

### \$upstream_last_first_byte_time
Keeps time spent on receiving the first byte from the latest upstream server; the time is kept in seconds with millisecond resolution.

### \$upstream_last_session_time
Keeps time spent on the session with the latest upstream server; the time is kept in seconds with millisecond resolution.

### \$upstream_last_bytes_received
Number of bytes received from the latest upstream server.

### \$upstream_last_bytes_sent
Number of bytes sent to the latest upstream server.

### \$upstream_total_bytes_received
Total number of bytes received from all upstream servers across all connection attempts.

### \$upstream_total_bytes_sent
Total number of bytes sent to all upstream servers across all connection attempts.

## Variables related to time-spent measurement

The following timestamp calculation logic can be used with `$msec`. The variables calculated using `$msec` have accurate values only when used in the access logs. Their values may not be accurate if used outside the log phase.

- \$session_time = \$msec - \$session_created_msec
- \$upstream_last_connect_time = \$upstream_last_connected_msec - \$upstream_last_start_msec
- \$upstream_last_first_byte_time = \$upstream_last_first_byte_msec - \$upstream_last_start_msec
- \$upstream_last_session_time = \$upstream_last_end_msec - \$upstream_last_start_msec

# Author

Jonathan Kolb
Hanada im@hanada.info

# License

This Nginx module is licensed under [BSD 2-Clause License](LICENSE).
