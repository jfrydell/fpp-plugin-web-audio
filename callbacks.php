#!/usr/bin/php
<?
if ($argc > 1)
{
	if ($argv[1] == "--list")
	{
		echo "media\n";
		exit(0);
	}
	else if ($argv[1] == "--type" && $argv[2] == "media")
	{
		$url = 'http://ec2-3-23-112-11.us-east-2.compute.amazonaws.com/lightshowstream/posttime.php';
		$data = array('time' => round(microtime(true)*1000));

		$options = array(
		    'http' => array(
		        'header'  => "Content-type: application/x-www-form-urlencoded\r\n",
		        'method'  => 'POST',
		        'content' => http_build_query($data)
		    )
		);
		$context  = stream_context_create($options);
		$result = file_get_contents($url, false, $context);
    }
}

?>
