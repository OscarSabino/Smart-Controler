<?php

// API para manejar las escenas (agrupar comandos)

require_once "conexion.php";

$metodo = $_SERVER['REQUEST_METHOD'];

// =============================================
// OBTENER TODAS LAS ESCENAS
// =============================================
if ($metodo == "GET") {

    $resultado = $conn->query("SELECT * FROM escenas ORDER BY id_escena DESC");
    $lista = [];

    while ($fila = $resultado->fetch_assoc()) {
        // Para cada escena buscamos sus comandos
        $id = $fila['id_escena'];
        $stmt = $conn->prepare("SELECT c.*, d.nombre AS dispositivo FROM escena_comandos ec JOIN comandos c ON ec.id_comando = c.id_comando JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo WHERE ec.id_escena = ?");
        $stmt->bind_param("i", $id);
        $stmt->execute();
        $comandos = $stmt->get_result()->fetch_all(MYSQLI_ASSOC);
        $stmt->close();
        $fila['comandos'] = $comandos;
        $lista[] = $fila;
    }

    echo json_encode($lista);

// =============================================
// CREAR UNA ESCENA
// =============================================
} elseif ($metodo == "POST") {

    $datos = json_decode(file_get_contents("php://input"), true);
    $nombre = $datos['nombre'];
    $comandos = $datos['comandos'] ?? [];

    if (empty($nombre)) {
        http_response_code(400);
        echo json_encode(["error" => "El nombre de la escena es obligatorio"]);
        exit();
    }

    // Primero creamos la escena
    $stmt = $conn->prepare("INSERT INTO escenas (nombre) VALUES (?)");
    $stmt->bind_param("s", $nombre);
    $stmt->execute();
    $id_escena = $conn->insert_id;
    $stmt->close();

    // Luego añadimos los comandos a la escena
    if (!empty($comandos)) {
        $stmt2 = $conn->prepare("INSERT INTO escena_comandos (id_escena, id_comando) VALUES (?, ?)");
        foreach ($comandos as $id_comando) {
            $stmt2->bind_param("ii", $id_escena, $id_comando);
            $stmt2->execute();
        }
        $stmt2->close();
    }

    http_response_code(201);
    echo json_encode(["mensaje" => "Escena creada correctamente", "id" => $id_escena]);

// =============================================
// BORRAR UNA ESCENA
// =============================================
} elseif ($metodo == "DELETE") {

    $id = $_GET['id'] ?? null;

    if (empty($id)) {
        http_response_code(400);
        echo json_encode(["error" => "Falta el ID de la escena"]);
        exit();
    }

    $stmt = $conn->prepare("DELETE FROM escena_comandos WHERE id_escena = ?");
    $stmt->bind_param("i", $id);
    $stmt->execute();
    $stmt->close();

    $stmt = $conn->prepare("DELETE FROM escenas WHERE id_escena = ?");
    $stmt->bind_param("i", $id);
    $stmt->execute();
    $stmt->close();

    echo json_encode(["mensaje" => "Escena eliminada correctamente"]);

} else {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
}

$conn->close();

?>
