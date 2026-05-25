-- =============================================
-- BASE DE DATOS DEL SMART CONTROLLER
-- =============================================

-- Primero creamos la base de datos
CREATE DATABASE IF NOT EXISTS smart_controller;
USE smart_controller;

-- =============================================
-- TABLA DE DISPOSITIVOS
-- =============================================
-- Aqui guardamos los dispositivos (TV, AC, luces...)
CREATE TABLE IF NOT EXISTS dispositivos (
    id_dispositivo INT AUTO_INCREMENT PRIMARY KEY,
    nombre VARCHAR(100) NOT NULL,
    tipo ENUM('IR', 'RF') NOT NULL,  -- IR = infrarrojo, RF = radiofrecuencia
    marca VARCHAR(100) DEFAULT '',
    modelo VARCHAR(100) DEFAULT '',
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =============================================
-- TABLA DE COMANDOS
-- =============================================
-- Cada comando es un boton de un dispositivo
CREATE TABLE IF NOT EXISTS comandos (
    id_comando INT AUTO_INCREMENT PRIMARY KEY,
    id_dispositivo INT NOT NULL,
    nombre VARCHAR(100) NOT NULL,  -- ej: "Encender", "Subir volumen"
    protocolo VARCHAR(20) NOT NULL DEFAULT 'IR',  -- IR o RF
    codigo VARCHAR(255) NOT NULL,  -- El codigo que se envia
    tipo ENUM('IR', 'RF') NOT NULL,
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_dispositivo) REFERENCES dispositivos(id_dispositivo) ON DELETE CASCADE
);

-- =============================================
-- TABLA DE ESCENAS
-- =============================================
-- Una escena agrupa varios comandos que se ejecutan juntos
CREATE TABLE IF NOT EXISTS escenas (
    id_escena INT AUTO_INCREMENT PRIMARY KEY,
    nombre VARCHAR(100) NOT NULL,
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =============================================
-- TABLA DE RELACION ESCENA-COMANDO
-- =============================================
-- Relaciona que comandos pertenecen a cada escena
CREATE TABLE IF NOT EXISTS escena_comandos (
    id_escena INT NOT NULL,
    id_comando INT NOT NULL,
    PRIMARY KEY (id_escena, id_comando),
    FOREIGN KEY (id_escena) REFERENCES escenas(id_escena) ON DELETE CASCADE,
    FOREIGN KEY (id_comando) REFERENCES comandos(id_comando) ON DELETE CASCADE
);

-- =============================================
-- TABLA DE PROGRAMACIONES
-- =============================================
-- Aqui guardamos los comandos programados por dia y hora
CREATE TABLE IF NOT EXISTS programaciones (
    id_programacion INT AUTO_INCREMENT PRIMARY KEY,
    id_comando INT NOT NULL,
    dia_semana ENUM('L','M','X','J','V','S','D','TODOS') NOT NULL,
    hora TIME NOT NULL,
    activo TINYINT DEFAULT 1,  -- 1 = activo, 0 = inactivo
    ultima_ejecucion DATETIME DEFAULT NULL,
    FOREIGN KEY (id_comando) REFERENCES comandos(id_comando) ON DELETE CASCADE
);

-- =============================================
-- DATOS DE PRUEBA
-- =============================================

-- Añadimos un par de dispositivos de ejemplo
INSERT INTO dispositivos (nombre, tipo, marca, modelo) VALUES
('TV Salon', 'IR', 'Samsung', 'UE43TU7100'),
('Aire Acondicionado', 'IR', 'Midea', 'MS12');

-- Añadimos comandos para cada dispositivo
-- TV Samsung (id_dispositivo = 1)
INSERT INTO comandos (id_dispositivo, nombre, protocolo, codigo, tipo) VALUES
(1, 'Encender', 'NEC', '0xE0E040BF', 'IR'),
(1, 'Apagar', 'NEC', '0xE0E040BF', 'IR'),
(1, 'Subir Volumen', 'NEC', '0xE0E0E01F', 'IR'),
(1, 'Bajar Volumen', 'NEC', '0xE0E0D02F', 'IR');

-- Aire Acondicionado (id_dispositivo = 2)
INSERT INTO comandos (id_dispositivo, nombre, protocolo, codigo, tipo) VALUES
(2, 'Encender', 'NEC', '0xBB44DD22', 'IR'),
(2, 'Apagar', 'NEC', '0xBB44DD22', 'IR'),
(2, 'Subir Temperatura', 'NEC', '0xBB44CC33', 'IR'),
(2, 'Bajar Temperatura', 'NEC', '0xBB44EE11', 'IR');

-- Una escena de ejemplo
INSERT INTO escenas (nombre) VALUES ('Ver Pelicula');

-- Asignamos comandos a la escena
INSERT INTO escena_comandos (id_escena, id_comando) VALUES
(1, 1),  -- Encender TV
(1, 5);  -- Encender AC
