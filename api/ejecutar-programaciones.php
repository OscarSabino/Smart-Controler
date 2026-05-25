<?php

// Revisa si hay programaciones que toque ejecutar ahora y las envia al Arduino

require_once "conexion.php";

if ($_SERVER['REQUEST_METHOD'] != "GET") {
    http_response_code(405);
    echo json_encode(["error" => "Metodo no permitido"]);
    exit();
}

$dias = ["Monday" => "L", "Tuesday" => "M", "Wednesday" => "X", "Thursday" => "J", "Friday" => "V", "Saturday" => "S", "Sunday" => "D"];
$dia_actual = $dias[date("l")];
$hora_actual = date("H:i:s");
$hora_inicio = date("H:i:s", strtotime("-1 minute"));
$momento_actual = date("Y-m-d H:i:s");
$momento_limite = date("Y-m-d H:i:s", strtotime("-23 hours"));

// Buscamos las programaciones activas cuya hora coincida (margen de 1 minuto)
// Usamos la hora de PHP (la de tu PC) para que coincida con tu hora local
$stmt = $conn->prepare("
    SELECT p.id_programacion, c.protocolo, c.codigo, c.nombre AS comando_nombre, d.nombre AS dispositivo_nombre
    FROM programaciones p
    JOIN comandos c ON p.id_comando = c.id_comando
    JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo
    WHERE (p.dia_semana = ? OR p.dia_semana = 'TODOS')
      AND p.hora BETWEEN ? AND ?
      AND p.activo = 1
      AND (p.ultima_ejecucion IS NULL OR p.ultima_ejecucion < ?)
");
$stmt->bind_param("ssss", $dia_actual, $hora_inicio, $hora_actual, $momento_limite);
$stmt->execute();
$resultado = $stmt->get_result();
$stmt->close();

$ejecutados = [];

while ($fila = $resultado->fetch_assoc()) {
    $data = json_encode([
        "protocolo" => $fila["protocolo"],
        "codigo" => $fila["codigo"]
    ]);

    // Enviamos el comando al servidor Python
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

    // Si se envio bien, marcamos la ultima ejecucion
    if ($exito) {
        $stmt2 = $conn->prepare("UPDATE programaciones SET ultima_ejecucion = ? WHERE id_programacion = ?");
        $stmt2->bind_param("si", $momento_actual, $fila["id_programacion"]);
        $stmt2->execute();
        $stmt2->close();
    }

    $ejecutados[] = [
        "id" => $fila["id_programacion"],
        "comando" => $fila["comando_nombre"],
        "dispositivo" => $fila["dispositivo_nombre"],
        "exito" => $exito,
        "mensaje" => $exito ? ($respuesta_json["mensaje"] ?? "Enviado") : ($respuesta_json["error"] ?? "Error de conexion con el servidor Python")
    ];
}

echo json_encode([
    "ejecutados" => $ejecutados,
    "dia" => $dia_actual,
    "hora" => date("H:i:s")
]);

$conn->close();
?>
