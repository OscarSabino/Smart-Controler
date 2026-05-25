# Smart Controller - Guía de instalación

Proyecto académico para controlar dispositivos IR y RF con Arduino Nano + HC-05.

## Estructura de carpetas

```
smart-controller/
├── api/
│   ├── conexion.php        ← Configuración de la BD
│   ├── dispositivos.php    ← API de dispositivos
│   └── comandos.php        ← API de comandos
├── web/
│   ├── index.html          ← Página de inicio
│   ├── dispositivos.html   ← Gestión de dispositivos
│   ├── comandos.html       ← Gestión de comandos
│   ├── js/
│   │   └── main.js         ← Toda la lógica JS
│   └── css/
│       └── style.css       ← Estilos básicos
├── base_de_datos.sql       ← Script SQL para crear la BD
└── README.md               ← Este archivo
```

## Pasos para instalar

### 1. Instalar XAMPP (o similar)
Descarga XAMPP desde https://www.apachefriends.org e instálalo.
Arranca los servicios **Apache** y **MySQL**.

### 2. Copiar los archivos
Copia toda la carpeta `smart-controller` dentro de:
```
C:\xampp\htdocs\smart-controller\   (Windows)
/opt/lampp/htdocs/smart-controller/ (Linux)
```

### 3. Crear la base de datos
1. Abre el navegador y ve a: `http://localhost/phpmyadmin`
2. Haz clic en **"Nueva"** para crear una base de datos
3. O directamente importa el archivo `base_de_datos.sql`:
   - Clic en **"Importar"** → selecciona el archivo → **"Continuar"**

### 4. Configurar la conexión
Abre `api/conexion.php` y ajusta estos valores si los tuyos son diferentes:
```php
$host     = "localhost";
$usuario  = "root";
$password = "";       // En XAMPP suele estar vacío
$base     = "smart_controller";
```

### 5. Ajustar la URL de la API en JavaScript
Abre `web/js/main.js` y asegúrate de que la URL apunta a tu servidor:
```javascript
const API_URL = "http://localhost/smart-controller/api";
```

### 6. Abrir la web
Ve a: `http://localhost/smart-controller/web/index.html`

---

## Endpoints de la API

| Método | URL                                      | Qué hace                        |
|--------|------------------------------------------|---------------------------------|
| GET    | /api/dispositivos.php                    | Lista todos los dispositivos    |
| POST   | /api/dispositivos.php                    | Añade un dispositivo            |
| DELETE | /api/dispositivos.php?id=3               | Borra un dispositivo (y sus comandos) |
| GET    | /api/comandos.php                        | Lista todos los comandos        |
| GET    | /api/comandos.php?id_dispositivo=2       | Lista comandos de un dispositivo|
| POST   | /api/comandos.php                        | Añade un comando                |
| DELETE | /api/comandos.php?id=5                   | Borra un comando                |

---

## Ejemplo de JSON para añadir un dispositivo (POST)
```json
{
    "nombre": "TV Salón",
    "tipo": "Television",
    "marca": "Samsung",
    "modelo": "UE43AU7105"
}
```

## Ejemplo de JSON para añadir un comando (POST)
```json
{
    "id_dispositivo": 1,
    "nombre": "Encender",
    "protocolo": "IR",
    "codigo": "0xE0E040BF",
    "tipo": "power"
}
```
