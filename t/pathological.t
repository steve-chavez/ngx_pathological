#!/usr/bin/perl

use warnings;
use strict;

use Test::More;
use Socket qw/ CRLF /;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http rewrite/)->plan(3);

$t->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /pathological {
            pathological;
        }
    }
}

EOF

$t->run();

# status

like(http_get('/pathological?status=202'), qr/202 Accepted/ms, 'standard status');
like(http_get('/pathological?status=999'), qr/999/ms, 'custom status');

# headers

my $response = http_get('/pathological?malformed-header=missing-value');
my ($missing_value) = $response =~ m{^MissingValue:\s*$}m;
ok(defined $missing_value, "MissingValue is present in the response");
