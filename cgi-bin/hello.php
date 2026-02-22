<?php
// Output CGI headers
header('Content-Type: text/html; charset=UTF-8');

// Get environment variables
$method = getenv('REQUEST_METHOD') ?: 'N/A';
$query = getenv('QUERY_STRING') ?: 'N/A';
$time = date('Y-m-d H:i:s');
$php_version = phpversion();
?>
<html>
<head><title>PHP CGI</title></head>
<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>
    <div style='text-align:center;'>
        <h1>🐘 PHP CGI Script</h1>
        <p>Server Time: <?php echo $time; ?></p>
        <p>PHP Version: <?php echo $php_version; ?></p>
        <p>Request Method: <?php echo $method; ?></p>
        <p>Query String: <?php echo htmlspecialchars($query); ?></p>
        <hr>
        <form action='/index.html' method='get'>
            <button type='submit'>Back to Home</button>
        </form>
    </div>
</body>
</html>
