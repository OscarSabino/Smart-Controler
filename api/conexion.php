<?php

// Datos para conectar a la base de datos en Clever Cloud
$host = "biq1triovshxopl57e4o-mysql.services.clever-cloud.com";
$usuario = "u58jjpp0ggsqo2kk";
$password = "SIem30XLkqRXFWA4bmtF";
$base = "biq1triovshxopl57e4o";
$puerto = 3306;

// Crear la conexion con MySQL
$conn = new mysqli($host, $usuario, $password, $base, $puerto);

// Si falla la conexion mostramos un error
if ($conn->connect_error) {
    http_response_code(500);
    echo json_encode(["error" => "Error de conexion: " . $conn->connect_error]);
    exit();
}

// Para que los acentos y eñes funcionen bien
$conn->set_charset("utf8");

// Permite que la web pueda llamar a la API desde cualquier lugar
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type");
header("Content-Type: application/json");

// Cuando el navegador hace una peticion OPTIONS (antes de enviar datos)
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

?>
