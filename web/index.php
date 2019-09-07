<meta charset="UTF-8">
<html>
<head>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
  <script src="https://www.gstatic.com/charts/loader.js"></script>
  <script src="chart.js"></script>
</head>

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
    echo "<div>Welcome " . $_COOKIE['user'] . "</div>";
?>

<div>
  <input type="button" value="<<" onclick="prev();" />
  <span id="date"></span>
  <input type="button" value=">>" onclick="next();" />
</div>

<h1>Shiatsu</h1>
<div id="switchStatus1"></div>
<div id="chart1"></div>

<hr>
<h1>Entree</h1>
<div id="switchStatus2"></div>
<div id="chart2"></div>

<hr>
<h1>Lora</h1>
<div id="chart_lora"></div>

<?php
  if ($_COOKIE['user'])
    echo "<a href='logout.php'>Logout</a>";
  else
    echo "<a href='login.php'>Login</a>";
?>

</body>
</html>
</meta>
