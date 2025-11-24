#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <iomanip>

using namespace std;

// --- 1. ESTRUCTURAS DE DATOS BASE ---

// Estructura Paciente: Indexada por DNI en la Tabla Hash (Búsqueda O(1))
struct Paciente
{
    int idPaciente;
    string dni;
    string nombreCompleto;
};

// Estructura Medico: Indexada por nombreCompleto en la Tabla Hash (Disponibilidad en tiempo real)
struct Medico
{
    int idMedico;
    string nombreCompleto;
    string especialidad;
    bool disponible;
};

// Estructura Cita: Contiene información clave para el ordenamiento
struct Cita
{
    int idCita;
    int idPaciente;
    int idMedico;
    string dniPaciente;
    string nombreMedico;
    string fecha; // Formato YYYY-MM-DD para comparación
    string hora;
    string especialidad;
    int prioridad; // 1 (Alta) a 5 (Baja)
    bool cancelada = false;
};

// --- 2. DATOS GLOBALES Y HASHING ---

// Tablas Hash (unordered_map) para búsqueda O(1) promedio [cite: 47]
unordered_map<string, Paciente> tablaPacientes; // Clave: DNI
unordered_map<string, Medico> tablaMedicos;     // Clave: nombreCompleto

// Vectores para la gestión de citas y lista de espera
vector<Cita> citasProgramadas;
vector<Cita> listaEspera;

// Tipo de alias para el comparador de ordenamiento
using CitaComparator = function<bool(const Cita &, const Cita &)>;

// Comparador Global: Prioridad (menor es más urgente) > Fecha > Hora (RF01.1)
CitaComparator globalComparator = [](const Cita &a, const Cita &b)
{
    if (a.prioridad != b.prioridad)
    {
        return a.prioridad < b.prioridad;
    }
    if (a.fecha != b.fecha)
    {
        return a.fecha < b.fecha;
    }
    return a.hora < b.hora;
};

// --- 3. ALGORITMOS DE ORDENAMIENTO (Quick Sort y Merge Sort) ---

// Implementación de Partición (Necesaria para Quick Sort)
int particion(vector<Cita> &arr, int low, int high, const CitaComparator &comp)
{
    Cita pivot = arr[high];
    int i = (low - 1);
    for (int j = low; j <= high - 1; j++)
    {
        if (comp(arr[j], pivot))
        {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[high]);
    return (i + 1);
}

/**
 * Quick Sort: Ordenamiento rápido, útil para ordenar citas por fecha o prioridad[cite: 45].
 * Complejidad: O(n log n) promedio (RNF01.1).
 */
void quickSort(vector<Cita> &arr, int low, int high, const CitaComparator &comp)
{
    if (low < high)
    {
        int pi = particion(arr, low, high, comp);
        quickSort(arr, low, pi - 1, comp);
        quickSort(arr, pi + 1, high, comp);
    }
}

// Implementación de Merge (Necesaria para Merge Sort)
void merge(vector<Cita> &arr, int l, int m, int r, const CitaComparator &comp)
{
    int n1 = m - l + 1;
    int n2 = r - m;

    vector<Cita> L(n1), R(n2);
    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2)
    {
        // Mantiene la estabilidad: si son iguales, se toma primero de L
        if (comp(L[i], R[j]) || (!comp(R[j], L[i]) && !comp(L[i], R[j])))
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1)
        arr[k++] = L[i++];
    while (j < n2)
        arr[k++] = R[j++];
}

/**
 * Merge Sort: Ordenamiento estable, ideal para consolidar citas de múltiples clínicas[cite: 46].
 */
void mergeSort(vector<Cita> &arr, int l, int r, const CitaComparator &comp)
{
    if (l >= r)
        return;
    int m = l + (r - l) / 2;
    mergeSort(arr, l, m, comp);
    mergeSort(arr, m + 1, r, comp);
    merge(arr, l, m, r, comp);
}

// --- 4. FUNCIONES DEL MÓDULO (Siguiendo el Diagrama de Flujo) ---

/**
 * Cargar citas del día y cargar información de pacientes y medicos (Inicio)
 */
void cargarDatos()
{
    // Simulación de carga de pacientes (Hashing)
    tablaPacientes["23127181"] = {101, "23127181", "Carlos Quispe"};
    tablaPacientes["21200622"] = {102, "21200622", "Moises Sacsara"};

    // Simulación de carga de médicos (Hashing)
    tablaMedicos["Dioses Zarate"] = {501, "Dioses Zarate", "Cardiología", true};
    tablaMedicos["Terrel Santos"] = {502, "Terrel Santos", "Pediatría", true};

    // Simulación de carga de citas
    citasProgramadas = {
        {1, 101, 501, "23127181", "Dioses Zarate", "2025-12-01", "10:00", "Cardiología", 2},
        {2, 102, 502, "21200622", "Terrel Santos", "2025-12-01", "09:00", "Pediatría", 1},
        {3, 101, 501, "23127181", "Dioses Zarate", "2025-12-02", "08:00", "Cardiología", 3}};
    listaEspera = {
        {4, 999, 0, "33445566", "N/A", "N/A", "N/A", "Medicina General", 1} // Prioridad Alta (1)
    };
    cout << "\n[INFO] Datos iniciales cargados." << endl;
}

/**
 * Lógica de Ordenamiento y Fusión (¿Citas provienen de una sola área?)
 */
void iniciarProcesamiento(bool multiplesAreas)
{
    cout << "\n--- INICIO DEL PROCESAMIENTO DE CITAS ---" << endl;
    if (multiplesAreas)
    { // Diagrama: No -> Fusionar todas las listas de citas
        cout << "[MERGE SORT] Consolidando y ordenando citas de multiples areas (estable)..." << endl;
        // Se ejecuta Merge Sort para consolidación estable (RF01.4)
        mergeSort(citasProgramadas, 0, citasProgramadas.size() - 1, globalComparator);
    }
    else
    { // Diagrama: Sí -> Ordenar citas
        cout << "[QUICK SORT] Ordenando citas del dia por prioridad y hora (rapido)..." << endl;
        // Se ejecuta Quick Sort para ordenamiento rápido (RF01.1)
        quickSort(citasProgramadas, 0, citasProgramadas.size() - 1, globalComparator);
    }
    cout << "[INFO] Citas procesadas correctamente." << endl;
}

/**
 * Buscar paciente (Diagrama: Buscar paciente -> Mostrar datos de paciente)
 * Usa Hashing O(1) promedio (RF01.2)
 */
void buscarPacientePorDNI()
{
    string dni;
    cout << "\n[BUSCAR PACIENTE] Ingrese DNI: ";
    cin >> dni;

    auto it = tablaPacientes.find(dni); // Búsqueda O(1)
    if (it != tablaPacientes.end())
    {
        cout << "  > ENCONTRADO: " << it->second.nombreCompleto << endl;
        cout << "  > DNI: " << it->second.dni << ", ID: " << it->second.idPaciente << endl;
    }
    else
    {
        cout << "  > ERROR: Paciente con DNI " << dni << " no encontrado." << endl;
    }
}

/**
 * Modificar disponibilidad (Diagrama: Modificar disponibilidad -> Actualizar estado del médico)
 * Usa Hashing O(1) promedio (RF01.5)
 */
void modificarDisponibilidadMedico()
{
    string nombreMedico;
    int estado;
    cout << "\n[MODIFICAR DISPONIBILIDAD] Ingrese nombre del Medico: ";
    cin.ignore();
    getline(cin, nombreMedico);
    cout << "  > Ingrese nuevo estado (1: DISPONIBLE, 0: NO DISPONIBLE): ";
    cin >> estado;

    auto it = tablaMedicos.find(nombreMedico); // Búsqueda O(1)

    if (it != tablaMedicos.end())
    {
        it->second.disponible = (estado == 1);
        cout << "  > EXITO: Disponibilidad de " << nombreMedico
             << " actualizada a " << (it->second.disponible ? "DISPONIBLE" : "NO DISPONIBLE") << "." << endl;
    }
    else
    {
        cout << "  > ERROR: Medico " << nombreMedico << " no encontrado." << endl;
    }
}

/**
 * Cancelar cita (Diagrama: Cancelar cita -> Actualizar agenda -> Mostrar cita como cancelada)
 * Incluye Reasignación Automática (RF01.3)
 */
void cancelarCita()
{
    int idCita;
    cout << "\n[CANCELAR CITA] Ingrese ID de la cita a cancelar: ";
    if (!(cin >> idCita))
        return;

    Cita *citaCancelada = nullptr;
    // 1. Actualizar agenda/Mostrar cita como cancelada
    for (Cita &cita : citasProgramadas)
    {
        if (cita.idCita == idCita && !cita.cancelada)
        {
            cita.cancelada = true;
            citaCancelada = &cita;
            cout << "  > EXITO: Cita #" << idCita << " marcada como cancelada." << endl;
            break;
        }
    }

    if (citaCancelada && !listaEspera.empty())
    {
        // 2. Reasignación automática (RF01.3): Usamos Quick Sort para asegurar la máxima prioridad
        quickSort(listaEspera, 0, listaEspera.size() - 1, globalComparator);

        Cita pacientePrioritario = listaEspera[0];

        // 3. Asignar el slot liberado al paciente prioritario
        citaCancelada->idPaciente = pacientePrioritario.idPaciente;
        citaCancelada->dniPaciente = pacientePrioritario.dniPaciente;
        citaCancelada->cancelada = false; // La cita está nuevamente activa

        // 4. Eliminar de lista de espera
        listaEspera.erase(listaEspera.begin());

        cout << "  > REASIGNACIÓN: Cita reasignada a paciente prioritario (DNI: "
             << citaCancelada->dniPaciente << ") en el slot liberado." << endl;
    }
    else if (citaCancelada)
    {
        cout << "  > INFO: No hay pacientes en lista de espera para reasignar el slot." << endl;
    }
    else
    {
        cout << "  > ERROR: Cita #" << idCita << " no encontrada o ya estaba cancelada." << endl;
    }
}

// --- 5. FUNCIÓN MAIN INTERACTIVA ---

void mostrarMenu()
{
    cout << "\n=========================================================" << endl;
    cout << "  GESTION DE CITAS MEDICAS" << endl;
    cout << "=========================================================" << endl;
    cout << "1. Buscar Paciente (Hashing - RF01.2)" << endl;
    cout << "2. Cancelar Cita (Actualizar Agenda y Reasignar - RF01.3)" << endl;
    cout << "3. Modificar Disponibilidad de Medico (Hashing - RF01.5)" << endl;
    cout << "4. Mostrar Citas Programadas" << endl;
    cout << "0. Salir" << endl;
    cout << "Ingrese su opcion: ";
}

int main()
{
    cargarDatos(); // (Inicio -> Cargar datos)
    // 2. Ejecutar el procesamiento inicial (Ordenamiento/Fusion) automaticamente
    // Se pide al usuario la decision de si vienen de multiples areas
    int tipo_procesamiento;
    cout << "\n[CONFIGURACION INICIAL]" << endl;
    cout << "Las citas provienen de multiples areas? (1: Si [Merge Sort], 0: No [Quick Sort]): ";

    if (!(cin >> tipo_procesamiento))
    {
        cout << "\n[ERROR] Entrada invalida. Terminando programa." << endl;
        return 1;
    }
    iniciarProcesamiento(tipo_procesamiento == 1);

    int opcion;
    do
    {
        mostrarMenu();
        if (!(cin >> opcion))
        {
            cout << "\n[ERROR] Entrada invalida. Saliendo..." << endl;
            break;
        }

        // Ingresar solicitudes del usuario
        switch (opcion)
        {
        case 1:
            // Implementa: Buscar paciente (Hashing)
            buscarPacientePorDNI();
            break;
        case 2:
            // Implementa: Cancelar cita (Actualizar agenda y reasignar)
            cancelarCita();
            break;
        case 3:
            // Implementa: Modificar disponibilidad (Actualizar estado del médico)
            modificarDisponibilidadMedico();
            break;
        case 4:
            cout << "\n--- LISTA DE CITAS PROGRAMADAS ---" << endl;
            for (const auto &cita : citasProgramadas)
            {
                string estado = cita.cancelada ? "CANCELADA" : "ACTIVA";
                cout << std::left << setw(5) << "ID:" << setw(5) << cita.idCita
                     << setw(15) << "Medico:" << setw(20) << cita.nombreMedico
                     << setw(12) << "Fecha:" << setw(12) << cita.fecha
                     << setw(10) << "Hora:" << setw(6) << cita.hora
                     << setw(15) << "Prioridad:" << setw(3) << cita.prioridad
                     << setw(10) << "Estado:" << estado << endl;
            }
            break;
        case 0:
            cout << "\nSaliendo del Modulo de Gestion de Citas." << endl;
            break;
        default:
            cout << "\n[ERROR] Opcion no valida. Intente de nuevo." << endl;
            break;
        }
    } while (opcion != 0);

    return 0;
}