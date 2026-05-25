<?php

require_once "conexion.php";

$metodo = $_SERVER['REQUEST_METHOD'];

// =============================================
// OBTENER COMANDOS
// =============================================
if ($metodo == "GET") {

    $id_dispositivo = $_GET['id_dispositivo'] ?? null;

    // Si nos pasan un id_dispositivo filtramos por ese dispositivo
    if ($id_dispositivo) {
        $stmt = $conn->prepare("SELECT * FROM comandos WHERE id_dispositivo = ? ORDER BY fecha_registro DESC");
        $stmt->bind_param("i", $id_dispositivo);
        $stmt->execute();
        $resultado = $stmt->get_result();
        $stmt->close();
    } else {
        // Si no, devolvemos todos los comandos
        $resultado = $conn->query("SELECT * FROM comandos ORDER BY fecha_registro DESC");
    }

    $lista = [];
    while ($fila = $resultado->fetch_assoc()) {
        $lista[] = $fila;
    }

    echo json_encode($lista);

// =============================================
// AÑADIR UN COMANDO
// =============================================
} elseif ($metodo == "POST") {

    $datos = json_decode(file_get_contents("php://input"), true);

    $id_dispositivo = $datos['id_dispositivo'];
    $nombre = $datos['nombre'];
    $protocolo = $datos['protocolo'] ?? "IR";
    $codigo = $datos['codigo'];

    // Validamos que los campos obligatorios esten rellenos
    if (empty($id_dispositivo) || empty($nombre) || empty($codigo)) {
        http_response_code(400);
        echo json_encode(["error" => "Faltan campos obligatorios"]);
        exit();
    }

    // El tipo lo ponemos igual que el protocolo (IR o RF)
    $tipo = $protocolo;

    $stmt = $conn->prepare("INSERT INTO comandos (id_dispositivo, nombre, protocolo, codigo, tipo, fecha_registro) VALUES (?, ?, ?, ?, ?, NOW())");
    $stmt->bind_param("issss", $id_dispositivo, $nombre, $protocolo, $codigo, $tipo);

    if ($stmt->execute()) {
        http_response_code(201);
        echo json_encode(["mensaje" => "Comando añadido correctamente", "id" => $conn->insert_id]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo añadir el comando"]);
    }

    $stmt->close();

// =============================================
// BORRAR UN COMANDO
// =============================================
} elseif ($metodo == "DELETE") {

    $id = $_GET['id'] ?? null;

    if (empty($id)) {
        http_response_code(400);
        echo json_encode(["error" => "Falta el ID del comando"]);
        exit();
    }

    $stmt = $conn->prepare("DELETE FROM comandos WHERE id_comando = ?");
    $stmt->bind_param("i", $id);

    if ($stmt->execute()) {
        echo json_encode(["mensaje" => "Comando eliminado correctamente"]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo eliminar el comando"]);
    }

    $stmt->close();

} else {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
}

$conn->close();

?>
