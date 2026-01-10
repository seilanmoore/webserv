#!/usr/bin/perl

use strict;
use warnings;

# Output CGI headers
print "Content-Type: text/html\n\n";

# Get environment variables
my $method = $ENV{'REQUEST_METHOD'} || 'N/A';
my $query = $ENV{'QUERY_STRING'} || 'N/A';

# Get current time
my @time = localtime();
my $time_str = sprintf("%04d-%02d-%02d %02d:%02d:%02d",
    $time[5]+1900, $time[4]+1, $time[3], $time[2], $time[1], $time[0]);

# Output HTML
print << "EOF";
<html>
<head><title>Perl CGI</title></head>
<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>
    <div style='text-align:center;'>
        <h1>🐪 Perl CGI Script</h1>
        <p>Server Time: $time_str</p>
        <p>Perl Version: $]</p>
        <p>Request Method: $method</p>
        <p>Query String: $query</p>
        <hr>
        <form action='/docs/html/index.html' method='get'>
            <button type='submit'>Back to Home</button>
        </form>
    </div>
</body>
</html>
EOF
