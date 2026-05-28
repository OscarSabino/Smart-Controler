// =============================================
// CONFIGURACION
// =============================================

// Si abrimos la pagina directamente (file://) usamos localhost
// Si la abrimos desde un servidor usamos la API relativa
if (window.location.protocol == "file:") {
    var API_URL = "http://localhost:8080/api";
} else {
    var API_URL = "../api";
}

// =============================================
// FUNCIONES PARA DISPOSITIVOS
// =============================================

// Cargar todos los dispositivos de la base de datos
function cargarDispositivos() {
    fetch(API_URL + "/dispositivos.php")
        .then(function(respuesta) {
            return respuesta.json();
        })
        .then(function(datos) {
            var tbody = document.getElementById("tbody-dispositivos");
            tbody.innerHTML = "";

            // Recorremos cada dispositivo y creamos una fila en la tabla
            for (var i = 0; i < datos.length; i++) {
                var d = datos[i];
                var fila = document.createElement("tr");

                // Ponemos los datos en cada celda
                fila.innerHTML =
                    "<td>" + d.id_dispositivo + "</td>" +
                    "<td>" + d.nombre + "</td>" +
                    "<td>" + d.tipo + "</td>" +
                    "<td>" + (d.marca || "-") + "</td>" +
                    "<td>" + (d.modelo || "-") + "</td>" +
                    "<td><button onclick='borrarDispositivo(" + d.id_dispositivo + ")'>Eliminar</button></td>";

                tbody.appendChild(fila);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar dispositivos:", error);
        });
}

// Guardar un dispositivo nuevo
function guardarDispositivo() {
    // Cogemos los valores del formulario
    var nombre = document.getElementById("inp-nombre").value;
    var marca = document.getElementById("inp-marca").value;
    var modelo = document.getElementById("inp-modelo").value;
    var tipo = document.getElementById("inp-tipo").value;

    // Validamos que el nombre no este vacio
    if (nombre == "") {
        alert("El nombre es obligatorio");
        return;
    }

    // Enviamos los datos a la API
    fetch(API_URL + "/dispositivos.php", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            nombre: nombre,
            marca: marca,
            modelo: modelo,
            tipo: tipo
        })
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            // Limpiamos el formulario
            document.getElementById("inp-nombre").value = "";
            document.getElementById("inp-marca").value = "";
            document.getElementById("inp-modelo").value = "";
            // Recargamos la lista
            cargarDispositivos();
        })
        .catch(function(error) {
            console.error("Error al guardar:", error);
        });
}

// Borrar un dispositivo
function borrarDispositivo(id) {
    if (!confirm("¿Seguro que quieres eliminar este dispositivo?")) {
        return;
    }

    fetch(API_URL + "/dispositivos.php?id=" + id, {
        method: "DELETE"
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            cargarDispositivos();
        })
        .catch(function(error) {
            console.error("Error al borrar:", error);
        });
}

// =============================================
// FUNCIONES PARA COMANDOS
// =============================================

// Cargar dispositivos en un select (para elegirlos)
function cargarDispositivosEnSelect(idSelect) {
    fetch(API_URL + "/dispositivos.php")
        .then(function(r) { return r.json(); })
        .then(function(datos) {
            var select = document.getElementById(idSelect);
            // Limpiamos el select pero dejamos la primera opcion
            while (select.options.length > 1) {
                select.remove(1);
            }

            // Añadimos cada dispositivo como opcion
            for (var i = 0; i < datos.length; i++) {
                var opcion = document.createElement("option");
                opcion.value = datos[i].id_dispositivo;
                opcion.textContent = datos[i].nombre + " (" + datos[i].tipo + ")";
                select.appendChild(opcion);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar dispositivos en select:", error);
        });
}

// Cargar todos los comandos
function cargarComandos() {
    var selectFiltro = document.getElementById("filtro-dispositivo");
    var id_dispositivo = "";

    // Si hay un filtro seleccionado, lo añadimos a la URL
    if (selectFiltro && selectFiltro.value != "") {
        id_dispositivo = "?id_dispositivo=" + selectFiltro.value;
    }

    fetch(API_URL + "/comandos.php" + id_dispositivo)
        .then(function(r) { return r.json(); })
        .then(function(datos) {
            var tbody = document.getElementById("tbody-comandos");
            tbody.innerHTML = "";

            for (var i = 0; i < datos.length; i++) {
                var c = datos[i];
                var fila = document.createElement("tr");

                fila.innerHTML =
                    "<td>" + c.id_comando + "</td>" +
                    "<td>" + (c.id_dispositivo || "-") + "</td>" +
                    "<td>" + c.nombre + "</td>" +
                    "<td>" + c.protocolo + "</td>" +
                    "<td>" + c.codigo + "</td>" +
                    "<td><button onclick='borrarComando(" + c.id_comando + ")'>Eliminar</button></td>";

                tbody.appendChild(fila);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar comandos:", error);
        });
}

// Guardar un comando nuevo
function guardarComando() {
    var id_dispositivo = document.getElementById("inp-cmd-dispositivo").value;
    var nombre = document.getElementById("inp-cmd-nombre").value;
    var codigo = document.getElementById("inp-cmd-codigo").value;
    var protocolo = document.getElementById("inp-cmd-protocolo").value;

    // Validamos que los campos necesarios esten rellenos
    if (id_dispositivo == "" || nombre == "" || codigo == "") {
        alert("Todos los campos son obligatorios");
        return;
    }

    fetch(API_URL + "/comandos.php", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            id_dispositivo: id_dispositivo,
            nombre: nombre,
            codigo: codigo,
            protocolo: protocolo
        })
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            document.getElementById("inp-cmd-nombre").value = "";
            document.getElementById("inp-cmd-codigo").value = "";
            cargarComandos();
        })
        .catch(function(error) {
            console.error("Error al guardar comando:", error);
        });
}

// Variables para controlar el escaneo
var escaneando = false;
var controlEscanear = null;
var scanCancelado = false;
var countdownInterval = null;
var segundosRestantes = 30;

// Escanear un codigo desde el Arduino
function escanearCodigo() {
    if (escaneando) return;

    var btn = document.getElementById("btn-escanear");
    var btnCancelar = document.getElementById("btn-cancelar-scan");
    var input = document.getElementById("inp-cmd-codigo");
    var msg = document.getElementById("msg-escanear");
    var countdownDisplay = document.getElementById("countdown-display");

    escaneando = true;
    scanCancelado = false;
    segundosRestantes = 30;
    var protocolo = document.getElementById("inp-cmd-protocolo").value;

    btn.disabled = true;
    btn.textContent = "Escaneando...";
    btnCancelar.style.display = "inline-block";
    input.value = "";
    msg.textContent = "Esperando señal...";
    msg.className = "msg-scan msg-escuchando";
    countdownDisplay.style.display = "block";
    countdownDisplay.textContent = "Tiempo restante: " + segundosRestantes + "s";

    countdownInterval = setInterval(function() {
        segundosRestantes--;
        countdownDisplay.textContent = "Tiempo restante: " + segundosRestantes + "s";
        if (segundosRestantes <= 0) {
            clearInterval(countdownInterval);
            countdownInterval = null;
            if (escaneando) {
                scanCancelado = false;
                controlEscanear.abort();
            }
        }
    }, 1000);

    controlEscanear = new AbortController();

    // Timeout de 5s si el servidor Python no responde
    var timeoutConectar = setTimeout(function() {
        if (!escaneando) return;
        controlEscanear.abort();
        msg.textContent = "Error: No se puede conectar con el servidor Python (localhost:5000)";
        msg.className = "msg-scan msg-error";
        alert("El servidor Python no esta funcionando.\nAbre una terminal y ejecuta: python servidor_bt.py");
        finalizarEscaneo();
    }, 5000);

    fetch("http://localhost:5000/escanear", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ protocolo: protocolo }),
        signal: controlEscanear.signal
    })
        .then(function(r) {
            clearTimeout(timeoutConectar);
            if (!escaneando) return;
            if (r.status == 408) {
                return r.json().then(function(d) { throw new Error(d.error); });
            }
            if (!r.ok) {
                return r.json().then(function(d) { throw new Error(d.error); });
            }
            return r.json();
        })
        .then(function(respuesta) {
            if (!escaneando) return;
            input.value = respuesta.codigo;
            msg.textContent = "Codigo capturado!";
            msg.className = "msg-scan msg-ok";
        })
        .catch(function(error) {
            clearTimeout(timeoutConectar);
            if (!escaneando) return;
            if (error.name == "AbortError") {
                if (!scanCancelado) {
                    msg.textContent = "Error: Tiempo de espera agotado (30s)";
                    msg.className = "msg-scan msg-error";
                }
            } else {
                msg.textContent = "Error: Al recibir la señal...";
                msg.className = "msg-scan msg-error";
            }
        })
        .finally(function() {
            finalizarEscaneo();
        });
}

function cancelarEscaneo() {
    if (!escaneando) return;
    scanCancelado = true;
    controlEscanear.abort();
    var msg = document.getElementById("msg-escanear");
    msg.textContent = "Escaneo cancelado";
    msg.className = "msg-scan msg-error";
    finalizarEscaneo();
}

function finalizarEscaneo() {
    if (countdownInterval) {
        clearInterval(countdownInterval);
        countdownInterval = null;
    }
    var btn = document.getElementById("btn-escanear");
    var btnCancelar = document.getElementById("btn-cancelar-scan");
    var countdownDisplay = document.getElementById("countdown-display");

    btn.disabled = false;
    btn.textContent = "Escanear";
    btnCancelar.style.display = "none";
    countdownDisplay.style.display = "none";
    escaneando = false;
    controlEscanear = null;
}

// Borrar un comando
function borrarComando(id) {
    if (!confirm("¿Seguro que quieres eliminar este comando?")) {
        return;
    }

    fetch(API_URL + "/comandos.php?id=" + id, {
        method: "DELETE"
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            cargarComandos();
        })
        .catch(function(error) {
            console.error("Error al borrar comando:", error);
        });
}

// =============================================
// FUNCIONES PARA CONTROL REMOTO
// =============================================

// Cargar los comandos del dispositivo seleccionado y crear botones
function cargarBotones() {
    var id_dispositivo = document.getElementById("sel-dispositivo").value;
    var contenedor = document.getElementById("controles");

    if (id_dispositivo == "") {
        contenedor.innerHTML = "<p>Selecciona un dispositivo para ver sus comandos</p>";
        return;
    }

    fetch(API_URL + "/comandos.php?id_dispositivo=" + id_dispositivo)
        .then(function(r) { return r.json(); })
        .then(function(datos) {
            contenedor.innerHTML = "";

            // Si no hay comandos, mostramos un mensaje
            if (datos.length == 0) {
                contenedor.innerHTML = "<p>Este dispositivo no tiene comandos todavia</p>";
                return;
            }

            // Creamos un boton por cada comando
            for (var i = 0; i < datos.length; i++) {
                var btn = document.createElement("button");
                btn.className = "btn-comando";
                btn.textContent = datos[i].nombre;
                // Guardamos los datos del comando en atributos personalizados
                btn.setAttribute("data-id", datos[i].id_comando);
                btn.setAttribute("data-codigo", datos[i].codigo);
                btn.setAttribute("data-protocolo", datos[i].protocolo);
                // Cuando se hace clic, enviamos la señal
                btn.onclick = function() {
                    enviarComando(this.getAttribute("data-protocolo"), this.getAttribute("data-codigo"));
                };
                contenedor.appendChild(btn);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar botones:", error);
        });
}

// Enviar un comando al Arduino via Bluetooth
function enviarComando(protocolo, codigo) {
    var mensajeDiv = document.getElementById("mensaje");
    mensajeDiv.textContent = "Enviando señal...";
    mensajeDiv.className = "mensaje mensaje-enviando";

    fetch("http://localhost:5000/enviar", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            protocolo: protocolo,
            codigo: codigo
        })
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            if (respuesta.mensaje) {
                mensajeDiv.textContent = "✓ " + respuesta.mensaje;
                mensajeDiv.className = "mensaje mensaje-ok";
            } else {
                mensajeDiv.textContent = "✗ " + respuesta.error;
                mensajeDiv.className = "mensaje mensaje-error";
            }
        })
        .catch(function(error) {
            mensajeDiv.textContent = "✗ Error de conexion con el servidor local del BT" ;
            mensajeDiv.className = "mensaje mensaje-error";
            console.error("Error al enviar:", error);
        });
}

// =============================================
// FUNCIONES PARA PROGRAMACIONES
// =============================================

// Cargar comandos en un select (para programaciones)
// Si pasas id_dispositivo solo muestra los comandos de ese dispositivo
function cargarComandosEnSelect(idSelect, id_dispositivo) {
    var url = API_URL + "/comandos.php";

    if (id_dispositivo) {
        url = url + "?id_dispositivo=" + id_dispositivo;
    }

    fetch(url)
        .then(function(r) { return r.json(); })
        .then(function(datos) {
            var select = document.getElementById(idSelect);

            // Limpiamos el select pero dejamos la primera opcion
            while (select.options.length > 1) {
                select.remove(1);
            }

            for (var i = 0; i < datos.length; i++) {
                var opcion = document.createElement("option");
                opcion.value = datos[i].id_comando;
                opcion.textContent = "#" + datos[i].id_comando + " - " + datos[i].nombre;
                select.appendChild(opcion);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar comandos en select:", error);
        });
}

// Cuando cambia el dispositivo, cargamos solo sus comandos
function filtrarComandosPorDispositivo() {
    var id_dispositivo = document.getElementById("inp-prog-dispositivo").value;
    cargarComandosEnSelect("inp-prog-comando", id_dispositivo);
}

// Cargar las programaciones de la base de datos
function cargarProgramaciones() {
    var filtro = document.getElementById("filtro-dia");
    var dia = "";

    if (filtro && filtro.value != "") {
        dia = "?dia=" + filtro.value;
    }

    fetch(API_URL + "/programaciones.php" + dia)
        .then(function(r) { return r.json(); })
        .then(function(datos) {
            var tbody = document.getElementById("tbody-programaciones");
            tbody.innerHTML = "";

            // Convertimos las letras de los dias a nombres completos
            var nombresDias = {"L":"Lunes","M":"Martes","X":"Miércoles","J":"Jueves","V":"Viernes","S":"Sábado","D":"Domingo","TODOS":"Todos los días"};

            for (var i = 0; i < datos.length; i++) {
                var p = datos[i];
                var fila = document.createElement("tr");

                // Checkbox para activar/desactivar
                var checked = "";
                if (p.activo == 1) {
                    checked = "checked";
                }

                // Convertimos la letra del dia a su nombre
                var diaNombre = nombresDias[p.dia_semana] || p.dia_semana;

                fila.innerHTML =
                    "<td>" + p.id_programacion + "</td>" +
                    "<td>" + (p.dispositivo || "-") + "</td>" +
                    "<td>" + (p.comando || "-") + "</td>" +
                    "<td>" + diaNombre + "</td>" +
                    "<td>" + p.hora.substring(0, 5) + "</td>" +
                    "<td><input type='checkbox' " + checked + " onchange='toggleProgramacion(" + p.id_programacion + ", this.checked)'></td>" +
                    "<td><button onclick='ejecutarProgramacionAhora(" + p.id_programacion + ", this)' style='background:#27ae60; margin-right:5px;'>Ejecutar</button><button onclick='borrarProgramacion(" + p.id_programacion + ")'>Eliminar</button></td>";

                tbody.appendChild(fila);
            }
        })
        .catch(function(error) {
            console.error("Error al cargar programaciones:", error);
        });
}

// Guardar una programacion nueva
function guardarProgramacion() {
    var id_comando = document.getElementById("inp-prog-comando").value;
    var dia = document.getElementById("inp-prog-dia").value;
    var hora = document.getElementById("inp-prog-hora").value;

    if (id_comando == "" || dia == "" || hora == "") {
        alert("Todos los campos son obligatorios");
        return;
    }

    fetch(API_URL + "/programaciones.php", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            id_comando: id_comando,
            dia: dia,
            hora: hora
        })
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            document.getElementById("inp-prog-hora").value = "";
            cargarProgramaciones();
        })
        .catch(function(error) {
            console.error("Error al guardar programacion:", error);
        });
}

// Activar o desactivar una programacion
function toggleProgramacion(id, activo) {
    var valor = 0;
    if (activo) {
        valor = 1;
    }

    fetch(API_URL + "/programaciones.php", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            id: id,
            activo: valor
        })
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            console.log(respuesta.mensaje);
        })
        .catch(function(error) {
            console.error("Error al actualizar:", error);
        });
}

// Borrar una programacion
function borrarProgramacion(id) {
    if (!confirm("¿Seguro que quieres eliminar esta programacion?")) {
        return;
    }

    fetch(API_URL + "/programaciones.php?id=" + id, {
        method: "DELETE"
    })
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            alert(respuesta.mensaje);
            cargarProgramaciones();
        })
        .catch(function(error) {
            console.error("Error al borrar programacion:", error);
        });
}

// =============================================
// VERIFICADOR DE PROGRAMACIONES
// =============================================

var intervaloVerificador = null;
var ejecucionesRecientes = [];

// Ejecuta una programacion manualmente desde el boton de la tabla
function ejecutarProgramacionAhora(id, btn) {
    btn.disabled = true;
    btn.textContent = "Enviando...";

    fetch(API_URL + "/ejecutar-programacion.php?id=" + id)
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            var icono = respuesta.exito ? "✓" : "✗";
            ejecucionesRecientes.unshift({
                hora: new Date().toLocaleTimeString(),
                texto: icono + " " + respuesta.dispositivo + " - " + respuesta.comando + " (" + respuesta.mensaje + ")"
            });
            if (ejecucionesRecientes.length > 50) ejecucionesRecientes.length = 50;
            actualizarEjecuciones();
            console.log(respuesta.exito ? "Comando enviado" : "Error: " + respuesta.mensaje);
        })
        .catch(function(error) {
            console.error("Error de conexion");
        })
        .finally(function() {
            btn.disabled = false;
            btn.textContent = "Ejecutar";
        });
}


// Cada 30s revisa si hay programaciones pendientes
function verificarProgramaciones() {
    fetch(API_URL + "/ejecutar-programaciones.php")
        .then(function(r) { return r.json(); })
        .then(function(respuesta) {
            if (respuesta.ejecutados && respuesta.ejecutados.length > 0) {
                for (var i = 0; i < respuesta.ejecutados.length; i++) {
                    var ejec = respuesta.ejecutados[i];
                    var icono = ejec.exito ? "✓" : "✗";
                    ejecucionesRecientes.unshift({
                        hora: new Date().toLocaleTimeString(),
                        texto: icono + " " + ejec.dispositivo + " - " + ejec.comando + " (" + ejec.mensaje + ")"
                    });
                }
                if (ejecucionesRecientes.length > 50) ejecucionesRecientes.length = 50;
                cargarProgramaciones();
            }
            actualizarEjecuciones();
        })
        .catch(function(error) {
            console.error("Error al verificar programaciones:", error);
        });
}

// Muestra el historial de ejecuciones en la pagina
function actualizarEjecuciones() {
    var logDiv = document.getElementById("log-ejecuciones");
    if (!logDiv) return;
    logDiv.innerHTML = "";

    if (ejecucionesRecientes.length == 0) {
        logDiv.innerHTML = "<div class='log-item'>Esperando ejecuciones programadas...</div>";
        return;
    }
    for (var i = 0; i < ejecucionesRecientes.length; i++) {
        var item = document.createElement("div");
        item.className = "log-item" + (ejecucionesRecientes[i].texto.indexOf("✓") == 0 ? " log-ok" : " log-error");
        item.textContent = ejecucionesRecientes[i].hora + " - " + ejecucionesRecientes[i].texto;
        logDiv.appendChild(item);
    }
}

function iniciarVerificadorProgramaciones() {
    detenerVerificadorProgramaciones();
    intervaloVerificador = setInterval(verificarProgramaciones, 30000);
    verificarProgramaciones();
}

function detenerVerificadorProgramaciones() {
    if (intervaloVerificador) {
        clearInterval(intervaloVerificador);
        intervaloVerificador = null;
    }
}
