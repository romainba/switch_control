<meta charset="UTF-8">
<html>
<head>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
  <script src="https://www.gstatic.com/charts/loader.js"></script>
  <script src="chart.js"></script>
</head>
<meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1">
<body>

<?php
  session_start();

  if (!isset($_SESSION['use'])) {
    if ($_COOKIE['user']) {
      $_SESSION['use'] = $_COOKIE['user'];
      setcookie('user', $_COOKIE['user'], time() + 60*60*24*360);
    }
  }
  if ($_COOKIE['user'])
    echo "<div style='font-size:3vw' align='center'>Welcome " .
       $_COOKIE['user'] . "</div>";
?>

<div style="font-size:3vw" align="center">
  <input type="button" value="<<" onclick="prev();" />
  <span id="date"></span>
  <input type="button" value=">>" onclick="next();" />
</div>
<br>

<div style="font-size:3vw" align="center">
     <b>Shiatsu</b>
     <label id="status1"</label>
</div>
<div id="chart1"></div>

<br>
<div style="font-size:3vw" align="center">
     <b>Entree</b>
     <label id="status2"></label>
</div>
<div id="chart2"></div>

<br>
<div style="font-size:3vw" align="center">
     <b>Lora</b>
     <label id="status3"</label>
</div>
<div id="chart3"></div>

<?php
  if ($_COOKIE['user'])
    echo "<a href='logout.php'>Logout</a>";
  else
    echo "<a href='login.php'>Login</a>";
?>

</body>
</html>
</meta>
