// Esperar a que el DOM esté cargado
document.addEventListener('DOMContentLoaded', () => {
    const boton = document.getElementById('miBoton');

    boton.addEventListener('click', () => {
        alert('¡Hola! El script de JavaScript está funcionando correctamente.');
        console.log('El usuario hizo clic en el botón.');
    });
});