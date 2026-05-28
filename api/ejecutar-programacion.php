<?php

// Ejecuta una programacion manualmente desde el boton "Ejecutar" de la tabla

require_once "conexion.php";

if ($_SERVER['REQUEST_METHOD'] != "GET") {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
    exit();
}

$id = $_GET['id'] ?? null;

if (empty($id)) {
    http_response_code(400);
    echo json_encode(["error" => "Falta el ID de la programacion"]);
    exit();
}

// Buscamos la programacion y su comando
$stmt = $conn->prepare("
    SELECT p.id_programacion, c.protocolo, c.codigo, c.nombre AS comando_nombre, d.nombre AS dispositivo_nombre
    FROM programaciones p
    JOIN comandos c ON p.id_comando = c.id_comando
    JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo
    WHERE p.id_programacion = ?
");
$stmt->bind_param("i", $id);
$stmt->execute();
$resultado = $stmt->get_result();
$fila = $resultado->fetch_assoc();
$stmt->close();

if (!$fila) {
    http_response_code(404);
    echo json_encode(["error" => "Programacion no encontrada"]);
    exit();
}

// Enviamos el comando al Arduino via el servidor Python
$data = json_encode([
    "protocolo" => $fila["protocolo"],
    "codigo" => $fila["codigo"]
]);

$contexto = stream_context_create([
    "http" => [
        "method" => "POST",
        "header" => "Content-Type: application/json\r\n",
        "content" => $data,
        "timeout" => 5
    ]
]);

$respuesta = @file_get_contents("http://localhost:5000/enviar", false, $contexto);
$respuesta_json = $respuesta ? json_decode($respuesta, true) : null;

$exito = $respuesta_json && !isset($respuesta_json["error"]);

if ($exito) {
    $ahora = date("Y-m-d H:i:s");
    $stmt2 = $conn->prepare("UPDATE programaciones SET ultima_ejecucion = ? WHERE id_programacion = ?");
    $stmt2->bind_param("si", $ahora, $id);
    $stmt2->execute();
    $stmt2->close();
}

echo json_encode([
    "id" => $fila["id_programacion"],
    "comando" => $fila["comando_nombre"],
    "dispositivo" => $fila["dispositivo_nombre"],
    "exito" => $exito,
    "mensaje" => $exito ? ($respuesta_json["mensaje"] ?? "Enviado") : ($respuesta_json["error"] ?? "Error de conexion con el servidor")
]);

$conn->close();
?>
