#!/usr/bin/ruby

# Output CGI headers
puts "Content-Type: text/html; charset=UTF-8"
puts ""

# Get environment variables
method = ENV['REQUEST_METHOD'] || 'N/A'
query = ENV['QUERY_STRING'] || 'N/A'
time = Time.now.strftime("%Y-%m-%d %H:%M:%S")
ruby_version = RUBY_VERSION

# Output HTML
puts <<EOF
<html>
<head><title>Ruby CGI</title></head>
<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>
    <div style='text-align:center;'>
        <h1>💎 Ruby CGI Script</h1>
        <p>Server Time: #{time}</p>
        <p>Ruby Version: #{ruby_version}</p>
        <p>Request Method: #{method}</p>
        <p>Query String: #{query}</p>
        <hr>
        <form action='/index.html' method='get'>
            <button type='submit'>Back to Home</button>
        </form>
    </div>
</body>
</html>
EOF
