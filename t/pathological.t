use lib 'lib';
use Test::Nginx::Socket;

plan tests => 3;

run_tests();

__DATA__

=== TEST 1: status
--- config
    location /pathological {
        pathological;
    }

--- request
    GET /pathological?status=999
--- error_code: 999

--- request
    GET /pathological?status=201
--- error_code: 201


=== TEST 2: headers
--- request
    GET /pathological?malformed-header=missing-value
--- response_headers
MissingValue:
