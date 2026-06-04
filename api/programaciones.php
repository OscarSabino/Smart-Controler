<?php

// API para manejar las programaciones (horarios para ejecutar comandos)

require_once "conexion.php";

$metodo = $_SERVER['REQUEST_METHOD'];

// =============================================
// OBTENER PROGRAMACIONES
// =============================================
if ($metodo == "GET") {

    $dia = $_GET['dia'] ?? null;

    // Si nos pasan un dia filtramos por ese dia
    if ($dia) {
        $stmt = $conn->prepare("SELECT p.*, c.nombre AS comando, d.nombre AS dispositivo FROM programaciones p JOIN comandos c ON p.id_comando = c.id_comando JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo WHERE p.dia_semana = ? OR p.dia_semana = 'TODOS' ORDER BY p.hora ASC");
        $stmt->bind_param("s", $dia);
        $stmt->execute();
        $resultado = $stmt->get_result();
        $stmt->close();
    } else {
        $resultado = $conn->query("SELECT p.*, c.nombre AS comando, d.nombre AS dispositivo FROM programaciones p JOIN comandos c ON p.id_comando = c.id_comando JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo ORDER BY p.hora ASC");
    }

    $lista = [];
    while ($fila = $resultado->fetch_assoc()) {
        $lista[] = $fila;
    }

    echo json_encode($lista);

// =============================================
// AÑADIR PROGRAMACION
// =============================================
} elseif ($metodo == "POST") {

    $datos = json_decode(file_get_contents("php://input"), true);

    $id_comando = $datos['id_comando'];
    $dia = $datos['dia'];
    $hora = $datos['hora'];
    $activo = $datos['activo'] ?? 1;

    if (empty($id_comando) || empty($dia) || empty($hora)) {
        http_response_code(400);
        echo json_encode(["error" => "Faltan campos obligatorios"]);
        exit();
    }

    // Convertimos el nombre del dia a la letra que usa la base de datos
    $mapa_dias = ["Lunes"=>"L","Martes"=>"M","Miércoles"=>"X","Jueves"=>"J","Viernes"=>"V","Sábado"=>"S","Domingo"=>"D"];
    $dia_letra = $mapa_dias[$dia] ?? $dia;

    $stmt = $conn->prepare("INSERT INTO programaciones (id_comando, dia_semana, hora, activo) VALUES (?, ?, ?, ?)");
    $stmt->bind_param("issi", $id_comando, $dia_letra, $hora, $activo);

    if ($stmt->execute()) {
        http_response_code(201);
        echo json_encode(["mensaje" => "Programacion añadida correctamente", "id" => $conn->insert_id]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo añadir la programacion"]);
    }

    $stmt->close();

// =============================================
// ACTUALIZAR PROGRAMACION (activar/desactivar)
// =============================================
} elseif ($metodo == "PATCH") {

    $datos = json_decode(file_get_contents("php://input"), true);
    $id = $datos['id'];
    $activo = $datos['activo'];

    if (empty($id) || !isset($activo)) {
        http_response_code(400);
        echo json_encode(["error" => "Faltan datos para actualizar"]);
        exit();
    }

    $stmt = $conn->prepare("UPDATE programaciones SET activo = ? WHERE id_programacion = ?");
    $stmt->bind_param("ii", $activo, $id);

    if ($stmt->execute()) {
        echo json_encode(["mensaje" => "Programacion actualizada correctamente"]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo actualizar"]);
    }

    $stmt->close();

// =============================================
// BORRAR PROGRAMACION
// =============================================
} elseif ($metodo == "DELETE") {

    $id = $_GET['id'] ?? null;

    if (empty($id)) {
        http_response_code(400);
        echo json_encode(["error" => "Falta el ID de la programacion"]);
        exit();
    }

    $stmt = $conn->prepare("DELETE FROM programaciones WHERE id_programacion = ?");
    $stmt->bind_param("i", $id);

    if ($stmt->execute()) {
        echo json_encode(["mensaje" => "Programacion eliminada correctamente"]);
    } else {
        http_response_code(500);
        echo json_encode(["error" => "No se pudo eliminar"]);
    }

    $stmt->close();

} else {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
}

$conn->close();

?>
