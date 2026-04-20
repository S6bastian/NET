#include <iostream>
#include <fstream> // Librería necesaria para manejar archivos

int main() {
    // Creamos una instancia de fstream para escritura
    std::ofstream archive("test.txt");

    // Verificamos si el archivo se abrió correctamente
    if (archive.is_open()) {
        for (int i = 0; i < 99998; ++i) {
            archive << 'X'; // Puedes cambiar 'X' por el caracter que prefieras
        }
        archive << 'O';
        // Cerramos el flujo del archivo
        archive.close();
        std::cout << "Se han escrito 99,999 caracteres en salida.txt con éxito." << std::endl;
    } else {
        std::cerr << "No se pudo abrir o crear el archivo." << std::endl;
        return 1;
    }

    return 0;
}