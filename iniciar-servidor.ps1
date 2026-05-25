# =============================================
# Formas de iniciar el Smart Controller
# =============================================

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "  SMART CONTROLLER" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "PASO 1 - Base de datos:" -ForegroundColor Green
Write-Host "  XAMPP: Arranca MySQL, ejecuta base_de_datos.sql en phpMyAdmin" -ForegroundColor White
Write-Host "  O configurar conexion.php con tu servidor MySQL" -ForegroundColor White
Write-Host ""

Write-Host "PASO 2 - Servidor web (PHP):" -ForegroundColor Green
Write-Host "  XAMPP: Arranca Apache, copia la carpeta a C:\xampp\htdocs\" -ForegroundColor White
Write-Host "  Abre http://localhost/smart-controller/web/index.html" -ForegroundColor White
Write-Host ""

Write-Host "PASO 3 - Servidor Bluetooth (Python):" -ForegroundColor Green
Write-Host "  Abre otra terminal y ejecuta:" -ForegroundColor White
Write-Host "  python servidor_bt.py" -ForegroundColor Yellow
Write-Host ""

Write-Host "PASO 4 - Configurar Bluetooth:" -ForegroundColor Green
Write-Host "  Empareja el HC-05 con Windows (BT_COM)" -ForegroundColor White
Write-Host "  Si no es COM4, pon el correcto:" -ForegroundColor White
Write-Host "  $env:BT_COM = "COM5" (en la terminal antes de python)" -ForegroundColor Yellow
Write-Host ""

Write-Host "OPCION 2 - PHP built-in server (si no usas XAMPP):" -ForegroundColor Yellow
$phpPath = "$env:TEMP\php\php.exe"
$projectRoot = "C:\Users\oscar\Desktop\WEB\smart-controller"
if (Test-Path $phpPath) {
    Write-Host "  PHP detectado en $phpPath" -ForegroundColor Green
    Write-Host "  Servidor web: http://localhost:8080/web/index.html" -ForegroundColor Cyan
    & $phpPath -S localhost:8080 -t $projectRoot
} else {
    Write-Host "  PHP no encontrado en $phpPath" -ForegroundColor Red
    Write-Host "  Usa XAMPP mejor" -ForegroundColor Yellow
}
