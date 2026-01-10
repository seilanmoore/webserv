#!/bin/bash

# Output CGI headers
echo "Content-Type: text/html"
echo ""

# Get values
SERVER_TIME=$(date)
HOST_NAME=$(hostname)

# Output HTML body
cat << EOF
<html>
<head><title>Bash CGI</title></head>
<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>
    <div style='text-align:center;'>
        <h1>🐚 Bash CGI Script</h1>
        <p>Server Time: ${SERVER_TIME}</p>
        <p>Hostname: ${HOST_NAME}</p>
        <p>Request Method: ${REQUEST_METHOD:-N/A}</p>
        <p>Query String: ${QUERY_STRING:-N/A}</p>
        <hr>
        <form action='/docs/html/index.html' method='get'>
            <button type='submit'>Back to Home</button>
        </form>
    </div>
</body>
</html>
EOF
