<meta charset="UTF-8">
<html>
<body>
<?php

require("control_mqtt.php");

//setState('OFF');
echo "sous-sol " . getState(1) . " " . getTemp(1) . "<br>";
echo "entree   " . getState(2) . " " . getTemp(2) . "<br>";
echo "lora     " . getTemp(3);

//toggleState();

?>
  </body>
</html>
</meta>
