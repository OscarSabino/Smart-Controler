<?php

// Este archivo envia la señal al Arduino por Bluetooth

require_once "conexion.php";

// Solo aceptamos peticiones POST
if ($_SERVER['REQUEST_METHOD'] != "POST") {
    http_response_code(405);
    echo json_encode(["error" => "Solo se aceptan peticiones POST"]);
    exit();
}

$datos = json_decode(file_get_contents("php://input"), true);
$id_comando = $datos['id_comando'];

if (empty($id_comando)) {
    http_response_code(400);
    echo json_encode(["error" => "Falta el id_comando"]);
    exit();
}

// Buscamos el comando en la base de datos junto con el nombre del dispositivo
$stmt = $conn->prepare("SELECT c.*, d.nombre AS dispositivo FROM comandos c JOIN dispositivos d ON c.id_dispositivo = d.id_dispositivo WHERE c.id_comando = ?");
$stmt->bind_param("i", $id_comando);
$stmt->execute();
$resultado = $stmt->get_result();
$comando = $resultado->fetch_assoc();
$stmt->close();

if (!$comando) {
    http_response_code(404);
    echo json_encode(["error" => "Comando no encontrado"]);
    exit();
}

// Validamos protocolo y codigo antes de pasarlos a PowerShell
$protocolo = strtoupper($comando['protocolo']);
$codigo = $comando['codigo'];
if (!in_array($protocolo, ['IR', 'RF']) || !preg_match('/^[A-Fa-f0-9]+$/', $codigo)) {
    http_response_code(400);
    echo json_encode(["error" => "Protocolo o codigo invalido"]);
    exit();
}

// Puerto COM donde esta conectado el HC-05 (cambiar segun tu PC)
$puerto = getenv("BT_COM") ?: "COM4";
$puerto_seguro = str_replace('"', '`"', $puerto);

// Preparamos la trama que enviaremos al Arduino: PROTOCOLO:CODIGO
$trama = $protocolo . ":" . $codigo . "\n";
$trama_segura = str_replace('"', '`"', $trama);

// Enviamos la trama al Arduino usando PowerShell (abre el puerto serie, escribe y cierra)
$comando_powershell = 'try { $p = New-Object System.IO.Ports.SerialPort("' . $puerto_seguro . '", 9600, None, 8, One); $p.Open(); $p.WriteLine("' . $trama_segura . '"); $p.Close(); Write-Host "OK" } catch { Write-Host "ERROR" }';
$salida = shell_exec("powershell -NoProfile -Command \"" . $comando_powershell . "\" 2>&1");

if (trim($salida) == "OK") {
    echo json_encode([
        "mensaje" => "Señal enviada correctamente",
        "comando" => $comando['nombre'],
        "dispositivo" => $comando['dispositivo'],
        "protocolo" => $comando['protocolo'],
        "codigo" => $comando['codigo']
    ]);
} else {
    http_response_code(500);
    echo json_encode([
        "error" => "No se pudo enviar la señal. ¿El Arduino esta conectado?",
        "codigo" => $comando['codigo']
    ]);
}

$conn->close();

?>
