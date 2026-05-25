<?php

// Incluimos la conexion a la base de datos
require_once "conexion.php";

// Vemos que tipo de peticion nos llega (GET, POST, DELETE...)
$metodo = $_SERVER['REQUEST_METHOD'];

// =============================================
// OBTENER TODOS LOS DISPOSITIVOS
// =============================================
if ($metodo == "GET") {

    $resultado = $conn->query("SELECT * FROM dispositivos ORDER BY id_dispositivo DESC");
    $lista = [];

    // Recorremos cada fila y la guardamos en el array
    while ($fila = $resultado->fetch_assoc()) {
        $lista[] = $fila;
    }

    echo json_encode($lista);

// =============================================
// AÑADIR UN NUEVO DISPOSITIVO
// =============================================
} elseif ($metodo == "POST") {

    // Leemos los datos que nos envian desde JavaScript
    $datos = json_decode(file_get_contents("php://input"), true);

    $nombre = $datos['nombre'];
    $tipo = $datos['tipo'];
    $marca = $datos['marca'] ?? "";
    $modelo = $datos['modelo'] ?? "";

    // Comprobamos que los campos obligatorios no esten vacios
    if (empty($nombre) || empty($tipo)) {
        http_response_code(400);
        echo json_encode(["error" => "El nombre y el tipo son obligatorios"]);
        exit();
    }

    // Insertamos el dispositivo en la base de datos
    $stmt = $conn->prepare("INSERT INTO dispositivos (nombre, tipo, marca, modelo) VALUES (?, ?, ?, ?)");
    $stmt->bind_param("ssss", $nombre, $tipo, $marca, $modelo);

    if ($stmt->execute()) {
        http_response_code(201);
        echo json_encode(["mensaje" => "Dispositivo añadido correctamente", "id" => $conn->insert_id]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo añadir el dispositivo"]);
    }

    $stmt->close();

// =============================================
// BORRAR UN DISPOSITIVO
// =============================================
} elseif ($metodo == "DELETE") {

    $id = $_GET['id'];

    if (empty($id)) {
        http_response_code(400);
        echo json_encode(["error" => "Falta el ID del dispositivo"]);
        exit();
    }

    // Borramos el dispositivo (los comandos se borran solos por la foreign key)
    $stmt = $conn->prepare("DELETE FROM dispositivos WHERE id_dispositivo = ?");
    $stmt->bind_param("i", $id);

    if ($stmt->execute()) {
        echo json_encode(["mensaje" => "Dispositivo eliminado correctamente"]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo eliminar el dispositivo"]);
    }

    $stmt->close();

// =============================================
// CUALQUIER OTRO METODO
// =============================================
} else {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
}

$conn->close();

?>
