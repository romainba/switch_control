<?php

session_start();
$pwd = '$2y$10$hkf00kcO/CdEGL0l6aaMzOlrzGP1P8ICdGqk5reS.EL8lKDKACmDG';
    
if (isset($_POST['login'])) {
    $user = $_POST['user'];
    $pass = $_POST['pass'];

    if ($user == "romain" && password_verify($pass, $pwd)) {
        $_SESSION['use']=$user;
        setcookie("user", $user, time() + 60*60*24*360);
        echo '<script type="text/javascript"> window.open("index.php","_self");</script>';
    } else {
        echo "invalid UserName or Password";        
    }
}
?>

<html>
<head>
<title> Login Page   </title>
</head>

<body>

<form action="" method="post">

    <table width="200" border="0">
  <tr>
    <td>  UserName</td>
    <td> <input type="text" name="user" > </td>
  </tr>
  <tr>
    <td> PassWord  </td>
    <td><input type="password" name="pass"></td>
  </tr>
  <tr>
    <td> <input type="submit" name="login" value="LOGIN"></td>
    <td></td>
  </tr>
</table>
</form>

</body>
</html>
