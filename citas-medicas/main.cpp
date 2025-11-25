#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <iomanip>
#include <queue>
#include <map> // Para inventario de medicamentos

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

// Tablas Hash (unordered_map) para búsqueda O(1) promedio
unordered_map<string, Paciente> tablaPacientes; // Clave: DNI
unordered_map<string, Medico> tablaMedicos;     // Clave: nombreCompleto

// Vectores para la gestión de citas y lista de espera
vector<Cita> citasProgramadas;
vector<Cita> listaEspera;

// Tipo de alias para el comparador de ordenamiento
using CitaComparator = function<bool(const Cita&, const Cita&)>;

// Comparador Global: Prioridad (menor es más urgente) > Fecha > Hora (RF01.1)
CitaComparator globalComparator = [](const Cita& a, const Cita& b)
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
int particion(vector<Cita>& arr, int low, int high, const CitaComparator& comp)
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
 * Quick Sort: Ordenamiento rápido, útil para ordenar citas por fecha o prioridad.
 * Complejidad: O(n log n) promedio.
 */
void quickSort(vector<Cita>& arr, int low, int high, const CitaComparator& comp)
{
    if (low < high)
    {
        int pi = particion(arr, low, high, comp);
        quickSort(arr, low, pi - 1, comp);
        quickSort(arr, pi + 1, high, comp);
    }
}

// Implementación de Merge (Necesaria para Merge Sort)
void merge(vector<Cita>& arr, int l, int m, int r, const CitaComparator& comp)
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
 * Merge Sort: Ordenamiento estable, ideal para consolidar citas de múltiples clínicas.
 */
void mergeSort(vector<Cita>& arr, int l, int r, const CitaComparator& comp)
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
    tablaPacientes["23127181"] = { 101, "23127181", "Carlos Quispe" };
    tablaPacientes["21200622"] = { 102, "21200622", "Moises Sacsara" };

    // Simulación de carga de médicos (Hashing)
    tablaMedicos["Dioses Zarate"] = { 501, "Dioses Zarate", "Cardiología", true };
    tablaMedicos["Terrel Santos"] = { 502, "Terrel Santos", "Pediatría", true };

    // Simulación de carga de citas
    citasProgramadas = {
        {1, 101, 501, "23127181", "Dioses Zarate", "2025-12-01", "10:00", "Cardiología", 2},
        {2, 102, 502, "21200622", "Terrel Santos", "2025-12-01", "09:00", "Pediatría", 1},
        {3, 101, 501, "23127181", "Dioses Zarate", "2025-12-02", "08:00", "Cardiología", 3} };
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
        // Se ejecuta Merge Sort para consolidación estable
        mergeSort(citasProgramadas, 0, citasProgramadas.size() - 1, globalComparator);
    }
    else
    { // Diagrama: Sí -> Ordenar citas
        cout << "[QUICK SORT] Ordenando citas del dia por prioridad y hora (rapido)..." << endl;
        // Se ejecuta Quick Sort para ordenamiento rápido
        quickSort(citasProgramadas, 0, citasProgramadas.size() - 1, globalComparator);
    }
    cout << "[INFO] Citas procesadas correctamente." << endl;
}

/**
 * Buscar paciente (Diagrama: Buscar paciente -> Mostrar datos de paciente)
 * Usa Hashing O(1) promedio.
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
 * Usa Hashing O(1) promedio.
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
 * Incluye Reasignación Automática.
 */
void cancelarCita()
{
    int idCita;
    cout << "\n[CANCELAR CITA] Ingrese ID de la cita a cancelar: ";
    if (!(cin >> idCita))
        return;

    Cita* citaCancelada = nullptr;
    // 1. Actualizar agenda/Mostrar cita como cancelada
    for (Cita& cita : citasProgramadas)
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
        // 2. Reasignación automática: Usamos Quick Sort para asegurar la máxima prioridad
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

// --- 4B. OPTIMIZACIÓN DE RUTA DE AMBULANCIA (GRAFO + BFS) ---

struct AristaAmbulancia
{
    int u;     // nodo origen
    int v;     // nodo destino
    int peso;  // tiempo estimado (minutos). Puede ser negativo como penalización.
};

int numNodosAmbulancia = 0;
vector<AristaAmbulancia> aristasAmbulancia;

// Modelo simple de ambulancias y emergencias
struct Ambulancia
{
    int idAmbulancia;
    int nodoActual;
    bool disponible;
};

struct EmergenciaRuta
{
    int idEmergencia;
    int nodoDestino;
    bool atendida;
    int idAmbulanciaAsignada;
    vector<int> ruta;
    int tiempoEstimado; // en minutos
};

vector<Ambulancia> ambulancias;
vector<EmergenciaRuta> emergencias;
int contadorEmergencias = 1;

const int INF_TIEMPO = 1000000000;

// Inicializa grafo urbano ponderado y ambulancias de ejemplo
void cargarGrafoAmbulancia()
{
    // Modelo urbano simple de 6 nodos
    // 0: Hospital Central
    // 1: Cruce Norte
    // 2: Cruce Sur
    // 3: Avenida Rapida
    // 4: Zona Residencial
    // 5: Zona Industrial
    numNodosAmbulancia = 6;

    aristasAmbulancia.clear();

    auto addEdge = [](int u, int v, int peso)
    {
        aristasAmbulancia.push_back({u, v, peso});
        aristasAmbulancia.push_back({v, u, peso}); // grafo no dirigido: se agrega en ambos sentidos
    };

    addEdge(0, 1, 5);   // Hospital -> Cruce Norte (5 min)
    addEdge(0, 2, 7);   // Hospital -> Cruce Sur (7 min)
    addEdge(1, 3, 3);   // Cruce Norte -> Av. Rapida (3 min)
    addEdge(2, 3, 2);   // Cruce Sur -> Av. Rapida (2 min)
    addEdge(3, 4, 6);   // Av. Rapida -> Zona Residencial (6 min)
    addEdge(3, 5, 4);   // Av. Rapida -> Zona Industrial (4 min)
    addEdge(4, 5, 5);   // Residencial <-> Industrial

    // Ambulancias disponibles en diferentes nodos de la ciudad
    ambulancias.clear();
    ambulancias.push_back({1, 0, true}); // Ambulancia 1 en Hospital Central
    ambulancias.push_back({2, 1, true}); // Ambulancia 2 en Cruce Norte
    ambulancias.push_back({3, 5, true}); // Ambulancia 3 en Zona Industrial

    cout << "\n[INFO] Grafo urbano ponderado de ambulancias cargado." << endl;
}

// Bellman-Ford: calcula distancias minimas desde un origen a todos los nodos
bool bellmanFord(int origen, vector<int>& dist, vector<int>& padre)
{
    dist.assign(numNodosAmbulancia, INF_TIEMPO);
    padre.assign(numNodosAmbulancia, -1);
    dist[origen] = 0;

    // Relajaciones
    for (int i = 0; i < numNodosAmbulancia - 1; ++i)
    {
        bool cambio = false;
        for (const auto& e : aristasAmbulancia)
        {
            if (dist[e.u] == INF_TIEMPO)
                continue;
            if (dist[e.u] + e.peso < dist[e.v])
            {
                dist[e.v] = dist[e.u] + e.peso;
                padre[e.v] = e.u;
                cambio = true;
            }
        }
        if (!cambio)
            break;
    }

    // Detección simple de ciclos negativos (no deberian existir en este modelo)
    for (const auto& e : aristasAmbulancia)
    {
        if (dist[e.u] == INF_TIEMPO)
            continue;
        if (dist[e.u] + e.peso < dist[e.v])
        {
            cout << "[ALERTA] Se detecto un posible ciclo negativo en el mapa urbano.\n";
            return false;
        }
    }
    return true;
}

// Construye la ruta desde origen a destino usando el vector padre
vector<int> reconstruirRuta(int origen, int destino, const vector<int>& padre)
{
    vector<int> ruta;
    int nodo = destino;
    while (nodo != -1)
    {
        ruta.push_back(nodo);
        if (nodo == origen)
            break;
        nodo = padre[nodo];
    }
    if (ruta.back() != origen)
    {
        ruta.clear(); // no hay ruta posible
        return ruta;
    }
    reverse(ruta.begin(), ruta.end());
    return ruta;
}

// Actualizar peso de una calle (para simular trafico, desvio, bloqueo, etc.)
void actualizarPesoArista(int u, int v, int nuevoPeso)
{
    bool encontrado = false;
    for (auto& e : aristasAmbulancia)
    {
        if ((e.u == u && e.v == v) || (e.u == v && e.v == u))
        {
            e.peso = nuevoPeso;
            encontrado = true;
        }
    }
    if (encontrado)
        cout << "[INFO] Peso de la arista (" << u << "," << v << ") actualizado a " << nuevoPeso << " minutos.\n";
    else
        cout << "[ADVERTENCIA] No se encontro una via entre " << u << " y " << v << ".\n";
}

// Registrar una nueva emergencia en el sistema
void registrarEmergencia()
{
    int nodo;
    cout << "\n[EMERGENCIA] Ingrese nodo de la emergencia (0-" << (numNodosAmbulancia - 1) << "): ";
    cin >> nodo;

    if (nodo < 0 || nodo >= numNodosAmbulancia)
    {
        cout << "[ERROR] Nodo fuera de rango.\n";
        return;
    }

    EmergenciaRuta e;
    e.idEmergencia = contadorEmergencias++;
    e.nodoDestino = nodo;
    e.atendida = false;
    e.idAmbulanciaAsignada = -1;
    e.tiempoEstimado = INF_TIEMPO;

    emergencias.push_back(e);

    cout << "[INFO] Emergencia #" << e.idEmergencia
         << " registrada en nodo " << e.nodoDestino << ".\n";
}

// Asignar la ambulancia mas cercana a una emergencia especifica
void asignarAmbulanciaMasCercana(int idEmergencia)
{
    if (emergencias.empty())
    {
        cout << "[INFO] No hay emergencias registradas.\n";
        return;
    }

    EmergenciaRuta* objetivo = nullptr;
    for (auto& e : emergencias)
    {
        if (e.idEmergencia == idEmergencia)
        {
            objetivo = &e;
            break;
        }
    }

    if (!objetivo)
    {
        cout << "[ERROR] Emergencia con ID " << idEmergencia << " no encontrada.\n";
        return;
    }
    if (objetivo->atendida)
    {
        cout << "[INFO] La emergencia #" << idEmergencia << " ya fue atendida.\n";
        return;
    }

    int mejorTiempo = INF_TIEMPO;
    int idxMejorAmb = -1;
    vector<int> mejorRuta;

    for (size_t i = 0; i < ambulancias.size(); ++i)
    {
        if (!ambulancias[i].disponible)
            continue;

        vector<int> dist, padre;
        if (!bellmanFord(ambulancias[i].nodoActual, dist, padre))
        {
            cout << "[ERROR] No se pudo ejecutar Bellman-Ford para la ambulancia " << ambulancias[i].idAmbulancia << ".\n";
            continue;
        }

        if (dist[objetivo->nodoDestino] < mejorTiempo)
        {
            vector<int> rutaCandidata = reconstruirRuta(ambulancias[i].nodoActual,
                                                        objetivo->nodoDestino, padre);
            if (!rutaCandidata.empty())
            {
                mejorTiempo = dist[objetivo->nodoDestino];
                idxMejorAmb = static_cast<int>(i);
                mejorRuta = rutaCandidata;
            }
        }
    }

    if (idxMejorAmb == -1)
    {
        cout << "[ALERTA] No hay ambulancias disponibles para esta emergencia.\n";
        return;
    }

    // Asignacion final
    objetivo->atendida = true;
    objetivo->idAmbulanciaAsignada = ambulancias[idxMejorAmb].idAmbulancia;
    objetivo->ruta = mejorRuta;
    objetivo->tiempoEstimado = mejorTiempo;
    ambulancias[idxMejorAmb].disponible = false;

    cout << "\n[ASIGNACION] Emergencia #" << objetivo->idEmergencia
         << " atendida por Ambulancia #" << objetivo->idAmbulanciaAsignada << ".\n";
    cout << "Ruta optima: ";
    for (int nodo : objetivo->ruta)
        cout << nodo << " ";
    cout << "\nTiempo estimado de llegada: " << objetivo->tiempoEstimado << " minutos.\n";
}

// Asignar ambulancias a todas las emergencias pendientes (emergencias simultaneas)
void asignarEmergenciasSimultaneas()
{
    if (emergencias.empty())
    {
        cout << "[INFO] No hay emergencias registradas.\n";
        return;
    }

    cout << "\n[EMERGENCIAS] Asignando ambulancias a todas las emergencias pendientes...\n";

    bool huboAsignaciones = false;

    // Estrategia greedy: siempre buscar la mejor ambulancia para cada emergencia pendiente
    for (auto& e : emergencias)
    {
        if (e.atendida)
            continue;

        int mejorTiempo = INF_TIEMPO;
        int idxMejorAmb = -1;
        vector<int> mejorRuta;

        for (size_t i = 0; i < ambulancias.size(); ++i)
        {
            if (!ambulancias[i].disponible)
                continue;

            vector<int> dist, padre;
            if (!bellmanFord(ambulancias[i].nodoActual, dist, padre))
                continue;

            if (dist[e.nodoDestino] < mejorTiempo)
            {
                vector<int> rutaCandidata = reconstruirRuta(ambulancias[i].nodoActual,
                                                            e.nodoDestino, padre);
                if (!rutaCandidata.empty())
                {
                    mejorTiempo = dist[e.nodoDestino];
                    idxMejorAmb = static_cast<int>(i);
                    mejorRuta = rutaCandidata;
                }
            }
        }

        if (idxMejorAmb != -1)
        {
            e.atendida = true;
            e.idAmbulanciaAsignada = ambulancias[idxMejorAmb].idAmbulancia;
            e.ruta = mejorRuta;
            e.tiempoEstimado = mejorTiempo;
            ambulancias[idxMejorAmb].disponible = false;
            huboAsignaciones = true;

            cout << "[ASIGNACION] Emergencia #" << e.idEmergencia
                 << " -> Ambulancia #" << e.idAmbulanciaAsignada
                 << " (ETA: " << e.tiempoEstimado << " min)\n";
        }
    }

    if (!huboAsignaciones)
        cout << "[INFO] No se pudo asignar ninguna ambulancia (todas ocupadas o sin ruta).\n";
}

// Permite actualizar el trafico y recalcular la ruta de una emergencia ya asignada
void actualizarTraficoYRecalcular()
{
    if (emergencias.empty())
    {
        cout << "[INFO] No hay emergencias registradas.\n";
        return;
    }

    int u, v, nuevoPeso;
    cout << "\n[TRAFICO] Actualizar peso de una via.\n";
    cout << "Nodo origen: ";
    cin >> u;
    cout << "Nodo destino: ";
    cin >> v;
    cout << "Nuevo tiempo estimado (minutos, puede ser negativo como penalizacion): ";
    cin >> nuevoPeso;

    actualizarPesoArista(u, v, nuevoPeso);

    int idEmerg;
    cout << "\n[RECALCULO] Ingrese ID de la emergencia a recalcular: ";
    cin >> idEmerg;

    EmergenciaRuta* e = nullptr;
    for (auto& em : emergencias)
    {
        if (em.idEmergencia == idEmerg)
        {
            e = &em;
            break;
        }
    }

    if (!e)
    {
        cout << "[ERROR] Emergencia no encontrada.\n";
        return;
    }

    if (e->idAmbulanciaAsignada == -1)
    {
        cout << "[INFO] La emergencia seleccionada aun no tiene ambulancia asignada.\n";
        return;
    }

    // Buscamos la ambulancia que tiene ese ID
    Ambulancia* amb = nullptr;
    for (auto& a : ambulancias)
    {
        if (a.idAmbulancia == e->idAmbulanciaAsignada)
        {
            amb = &a;
            break;
        }
    }

    if (!amb)
    {
        cout << "[ERROR] No se encontro la ambulancia asignada.\n";
        return;
    }

    vector<int> dist, padre;
    if (!bellmanFord(amb->nodoActual, dist, padre))
    {
        cout << "[ERROR] No se pudo recalcular la ruta.\n";
        return;
    }

    vector<int> nuevaRuta = reconstruirRuta(amb->nodoActual, e->nodoDestino, padre);
    if (nuevaRuta.empty())
    {
        cout << "[ALERTA] Tras el cambio de trafico ya no existe ruta valida hacia la emergencia.\n";
        return;
    }

    e->ruta = nuevaRuta;
    e->tiempoEstimado = dist[e->nodoDestino];

    cout << "\n[NUEVA RUTA] Emergencia #" << e->idEmergencia
         << " atendida por Ambulancia #" << e->idAmbulanciaAsignada << ".\n";
    cout << "Ruta recalculada: ";
    for (int nodo : e->ruta)
        cout << nodo << " ";
    cout << "\nNuevo tiempo estimado de llegada: " << e->tiempoEstimado << " minutos.\n";
}

// Mostrar resumen de emergencias registradas y tiempos estimados
void mostrarResumenEmergencias()
{
    if (emergencias.empty())
    {
        cout << "\n[INFO] No hay emergencias registradas.\n";
        return;
    }

    cout << "\n========== RESUMEN DE EMERGENCIAS ==========\n";
    for (const auto& e : emergencias)
    {
        cout << "Emergencia #" << e.idEmergencia
             << " | Nodo: " << e.nodoDestino
             << " | Estado: " << (e.atendida ? "ATENDIDA" : "PENDIENTE");

        if (e.atendida)
        {
            cout << " | Ambulancia #" << e.idAmbulanciaAsignada
                 << " | ETA: " << e.tiempoEstimado << " min";
        }
        cout << "\n";
    }
    cout << "===========================================\n";
}

// Sub-menu principal del modulo de rutas de ambulancia
void optimizarRutaAmbulancia()
{
    int opcion = -1;
    do
    {
        cout << "\n---------------- MODULO DE RUTAS DE AMBULANCIAS ----------------\n";
        cout << "1. Registrar nueva emergencia\n";
        cout << "2. Asignar ambulancia mas cercana a una emergencia\n";
        cout << "3. Asignar ambulancias a todas las emergencias pendientes\n";
        cout << "4. Actualizar trafico y recalcular ruta de una emergencia\n";
        cout << "5. Ver resumen de emergencias y tiempos estimados\n";
        cout << "0. Volver al menu principal\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        switch (opcion)
        {
        case 1:
            registrarEmergencia();
            break;
        case 2:
        {
            int id;
            cout << "Ingrese ID de la emergencia a asignar: ";
            cin >> id;
            asignarAmbulanciaMasCercana(id);
            break;
        }
        case 3:
            asignarEmergenciasSimultaneas();
            break;
        case 4:
            actualizarTraficoYRecalcular();
            break;
        case 5:
            mostrarResumenEmergencias();
            break;
        case 0:
            cout << "[INFO] Volviendo al menu principal...\n";
            break;
        default:
            cout << "[ERROR] Opcion no valida en modulo de ambulancias.\n";
            break;
        }

    } while (opcion != 0);
}

// --- 6. ASIGNACIÓN DE RECURSOS HOSPITALARIOS (GREEDY + SKEW HEAP) ---

// Niveles de urgencia ESI 1-5 (1 = más urgente)
enum class UrgenciaESI
{
    ESI1 = 1,
    ESI2 = 2,
    ESI3 = 3,
    ESI4 = 4,
    ESI5 = 5
};

// Paciente crítico para asignación de UCI/ventiladores
struct PacienteCritico
{
    int idPaciente;
    string dni;
    string nombre;
    UrgenciaESI urgencia;
    bool necesitaUCI;
    bool necesitaVentilador;
    bool necesitaMedCritica;
    long long ordenLlegada; // para desempatar por orden de llegada
};

// Comparador de prioridad: menor ESI -> más prioridad; si empatan, menor ordenLlegada
bool tieneMayorPrioridad(const PacienteCritico& a, const PacienteCritico& b)
{
    if (static_cast<int>(a.urgencia) != static_cast<int>(b.urgencia))
        return static_cast<int>(a.urgencia) < static_cast<int>(b.urgencia);
    return a.ordenLlegada < b.ordenLlegada;
}

// Nodo de Skew Heap
struct NodoSkew
{
    PacienteCritico valor;
    NodoSkew* izq;
    NodoSkew* der;

    NodoSkew(const PacienteCritico& p) : valor(p), izq(nullptr), der(nullptr) {}
};

// Skew Heap para cola de prioridad de pacientes críticos
class SkewHeapCritico
{
private:
    NodoSkew* raiz;

    static NodoSkew* fusionar(NodoSkew* a, NodoSkew* b)
    {
        if (!a)
            return b;
        if (!b)
            return a;

        // El nodo con mayor prioridad (más urgente) debe ir en la raíz
        if (!tieneMayorPrioridad(a->valor, b->valor))
        {
            std::swap(a, b);
        }

        // Fusión en subárbol derecho
        a->der = fusionar(a->der, b);

        // Propiedad del skew heap: se intercambian hijos
        std::swap(a->izq, a->der);
        return a;
    }

    static void limpiar(NodoSkew* n)
    {
        if (!n)
            return;
        limpiar(n->izq);
        limpiar(n->der);
        delete n;
    }

public:
    SkewHeapCritico() : raiz(nullptr) {}

    ~SkewHeapCritico()
    {
        limpiar(raiz);
    }

    bool estaVacio() const
    {
        return raiz == nullptr;
    }

    const PacienteCritico& obtenerMaxPrioridad() const
    {
        return raiz->valor; // se asume que no está vacío
    }

    void insertar(const PacienteCritico& p)
    {
        NodoSkew* n = new NodoSkew(p);
        raiz = fusionar(raiz, n);
    }

    void eliminarMaxPrioridad()
    {
        if (!raiz)
            return;
        NodoSkew* viejaRaiz = raiz;
        raiz = fusionar(raiz->izq, raiz->der);
        delete viejaRaiz;
    }
};

// Inventario de medicamentos críticos
struct StockMedicamento
{
    string nombre;
    int cantidad;
    int umbralMinimo;
};

class InventarioMedicamentos
{
private:
    map<string, StockMedicamento> inventario;

public:
    void agregarTipoMedicamento(const string& nombre, int cantidadInicial, int umbralMinimo)
    {
        inventario[nombre] = { nombre, cantidadInicial, umbralMinimo };
    }

    bool consumir(const string& nombre, int cantidad)
    {
        auto it = inventario.find(nombre);
        if (it == inventario.end())
        {
            cout << "[ALERTA] Medicamento '" << nombre << "' no registrado.\n";
            return false;
        }
        if (cantidad > it->second.cantidad)
        {
            cout << "[ALERTA] Stock insuficiente de '" << nombre
                << "'. Solicitado: " << cantidad
                << ", disponible: " << it->second.cantidad << "\n";
            return false;
        }
        it->second.cantidad -= cantidad;
        if (it->second.cantidad < it->second.umbralMinimo)
        {
            cout << "[ALERTA CRITICA] Stock bajo de '" << nombre
                << "'. Cantidad actual: " << it->second.cantidad << "\n";
        }
        return true;
    }

    void reabastecer(const string& nombre, int cantidad)
    {
        auto it = inventario.find(nombre);
        if (it == inventario.end())
        {
            // si no existe, se crea con umbral por defecto
            inventario[nombre] = { nombre, cantidad, max(1, cantidad / 4) };
        }
        else
        {
            it->second.cantidad += cantidad;
        }
        cout << "[INFO] Nuevo stock de '" << nombre
            << "': " << inventario[nombre].cantidad << "\n";
    }

    void reporte() const
    {
        cout << "=== INVENTARIO DE MEDICAMENTOS CRITICOS ===\n";
        for (const auto& kv : inventario)
        {
            const auto& m = kv.second;
            cout << " - " << m.nombre
                << " | Cant: " << m.cantidad
                << " | Umbral min: " << m.umbralMinimo << "\n";
        }
        cout << "===========================================\n";
    }
};

// Gestor de recursos hospitalarios (UCI, ventiladores, meds)
class GestorRecursosHospitalarios
{
private:
    int totalCamasUCI;
    int camasOcupadas;
    int totalVentiladores;
    int ventiladoresOcupados;

    long long contadorLlegada;
    SkewHeapCritico colaEspera;
    InventarioMedicamentos inventario;

public:
    GestorRecursosHospitalarios(int camasUCI, int ventiladores)
        : totalCamasUCI(camasUCI),
        camasOcupadas(0),
        totalVentiladores(ventiladores),
        ventiladoresOcupados(0),
        contadorLlegada(0) {}

    InventarioMedicamentos& obtenerInventario()
    {
        return inventario;
    }

    // Registro interactivo de paciente crítico
    void registrarPacienteCriticoInteractivo()
    {
        string dni;
        cout << "\n[RECURSOS] Ingrese DNI del paciente critico: ";
        cin >> dni;

        int idPaciente = -1;
        string nombre;

        auto it = tablaPacientes.find(dni);
        if (it != tablaPacientes.end())
        {
            idPaciente = it->second.idPaciente;
            nombre = it->second.nombreCompleto;
            cout << "  > Paciente encontrado en tabla: " << nombre << " (ID " << idPaciente << ")\n";
        }
        else
        {
            cout << "  > Paciente no registrado en tabla. Ingrese nombre completo: ";
            cin.ignore();
            getline(cin, nombre);
        }

        int nivelESI;
        char cUCI, cVent, cMed;
        cout << "  > Nivel de urgencia ESI (1-5): ";
        cin >> nivelESI;
        cout << "  > ¿Necesita cama UCI? (s/n): ";
        cin >> cUCI;
        cout << "  > ¿Necesita ventilador? (s/n): ";
        cin >> cVent;
        cout << "  > ¿Requiere medicacion critica inmediata? (s/n): ";
        cin >> cMed;

        UrgenciaESI urg;
        if (nivelESI <= 1)
            urg = UrgenciaESI::ESI1;
        else if (nivelESI == 2)
            urg = UrgenciaESI::ESI2;
        else if (nivelESI == 3)
            urg = UrgenciaESI::ESI3;
        else if (nivelESI == 4)
            urg = UrgenciaESI::ESI4;
        else
            urg = UrgenciaESI::ESI5;

        PacienteCritico p;
        p.idPaciente = idPaciente;
        p.dni = dni;
        p.nombre = nombre;
        p.urgencia = urg;
        p.necesitaUCI = (cUCI == 's' || cUCI == 'S');
        p.necesitaVentilador = (cVent == 's' || cVent == 'S');
        p.necesitaMedCritica = (cMed == 's' || cMed == 'S');
        p.ordenLlegada = contadorLlegada++;

        colaEspera.insertar(p);

        cout << "[INFO] Paciente critico " << p.nombre
            << " agregado a la cola de asignacion de recursos.\n";
    }

    // Asignación Greedy: siempre al paciente más urgente mientras haya recursos
    void asignarRecursosGreedy()
    {
        if (colaEspera.estaVacio())
        {
            cout << "\n[RECURSOS] No hay pacientes criticos en espera.\n";
            return;
        }

        cout << "\n[RECURSOS] Ejecutando asignacion de recursos (Greedy + Skew Heap)...\n";

        while (!colaEspera.estaVacio())
        {
            PacienteCritico top = colaEspera.obtenerMaxPrioridad();

            bool puedeUCI = (!top.necesitaUCI) || (camasOcupadas < totalCamasUCI);
            bool puedeVent = (!top.necesitaVentilador) || (ventiladoresOcupados < totalVentiladores);

            if (!puedeUCI || !puedeVent)
            {
                cout << "[INFO] Recursos insuficientes para asignar al siguiente paciente de mayor prioridad.\n";
                break;
            }

            cout << "[ASIGNACION] Paciente " << top.nombre << " (DNI: " << top.dni << ")\n";
            cout << "    - Urgencia ESI: " << static_cast<int>(top.urgencia) << "\n";

            if (top.necesitaUCI)
            {
                camasOcupadas++;
                cout << "    - Cama UCI asignada. (" << camasOcupadas
                    << "/" << totalCamasUCI << " ocupadas)\n";
            }

            if (top.necesitaVentilador)
            {
                ventiladoresOcupados++;
                cout << "    - Ventilador asignado. (" << ventiladoresOcupados
                    << "/" << totalVentiladores << " ocupados)\n";
            }

            if (top.necesitaMedCritica)
            {
                string med = "MedicamentoCriticoA";
                bool ok = inventario.consumir(med, 1);
                if (!ok)
                {
                    cout << "    [ALERTA] No se pudo dispensar medicacion critica para este paciente.\n";
                }
            }

            cout << "---------------------------------------------\n";

            colaEspera.eliminarMaxPrioridad();
        }
    }


    // Liberar recursos cuando pacientes salen de UCI/ventilador
    void darAltaPacientesInteractivo()
    {
        int liberarCamas, liberarVent;
        cout << "\n[ALTA PACIENTES] ¿Cuantas camas UCI se liberan?: ";
        cin >> liberarCamas;
        cout << "[ALTA PACIENTES] ¿Cuantos ventiladores se liberan?: ";
        cin >> liberarVent;

        camasOcupadas -= liberarCamas;
        ventiladoresOcupados -= liberarVent;

        if (camasOcupadas < 0)
            camasOcupadas = 0;
        if (ventiladoresOcupados < 0)
            ventiladoresOcupados = 0;

        cout << "[INFO] Recursos actualizados tras las altas.\n";
    }

    void reporteRecursos() const
    {
        cout << "\n========== ESTADO DE RECURSOS HOSPITALARIOS ==========\n";
        cout << "Camas UCI: " << camasOcupadas << " / " << totalCamasUCI << "\n";
        cout << "Ventiladores: " << ventiladoresOcupados << " / " << totalVentiladores << "\n";
        cout << "======================================================\n";
        inventario.reporte();
    }

    void reabastecerMedicamentoInteractivo()
    {
        string nombre;
        int cantidad;
        cout << "\n[REABASTECER] Nombre del medicamento critico: ";
        cin >> nombre;
        cout << "  > Cantidad a agregar: ";
        cin >> cantidad;
        inventario.reabastecer(nombre, cantidad);
    }
};

// Gestor global de recursos (ejemplo: 10 camas UCI y 6 ventiladores)
GestorRecursosHospitalarios gestorRecursos(10, 6);

// --- 5. FUNCIÓN MAIN INTERACTIVA ---

void mostrarMenu()
{
    cout << "\n=========================================================" << endl;
    cout << "  GESTION DE CITAS MEDICAS Y RECURSOS HOSPITALARIOS" << endl;
    cout << "=========================================================" << endl;
    cout << "1. Buscar Paciente (Hashing - RF01.2)" << endl;
    cout << "2. Cancelar Cita (Actualizar Agenda y Reasignar - RF01.3)" << endl;
    cout << "3. Modificar Disponibilidad de Medico (Hashing - RF01.5)" << endl;
    cout << "4. Mostrar Citas Programadas" << endl;
    cout << "5. Optimizar Ruta de Ambulancia (Grafo + BFS)" << endl;
    cout << "6. Registrar Paciente Critico (Cola Skew Heap)" << endl;
    cout << "7. Asignar Recursos (Greedy: UCI/Ventiladores + Meds)" << endl;
    cout << "8. Dar de Alta Pacientes (Liberar Recursos)" << endl;
    cout << "9. Ver Reporte de Recursos e Inventario" << endl;
    cout << "10. Reabastecer Medicamento Critico" << endl;
    cout << "0. Salir" << endl;
    cout << "Ingrese su opcion: ";
}

int main()
{
    cargarDatos(); // (Inicio -> Cargar datos)

    // Carga el grafo base de rutas para ambulancias
    cargarGrafoAmbulancia();

    // Configuracion inicial de procesamiento de citas
    int tipo_procesamiento;
    cout << "\n[CONFIGURACION INICIAL]" << endl;
    cout << "Las citas provienen de multiples areas? (1: Si [Merge Sort], 0: No [Quick Sort]): ";

    if (!(cin >> tipo_procesamiento))
    {
        cout << "\n[ERROR] Entrada invalida. Terminando programa." << endl;
        return 1;
    }
    iniciarProcesamiento(tipo_procesamiento == 1);

    // Configuracion inicial de inventario de medicamentos criticos
    InventarioMedicamentos& inv = gestorRecursos.obtenerInventario();
    inv.agregarTipoMedicamento("MedicamentoCriticoA", 5, 2);
    inv.agregarTipoMedicamento("MedicamentoCriticoB", 10, 3);

    int opcion;
    do
    {
        mostrarMenu();
        if (!(cin >> opcion))
        {
            cout << "\n[ERROR] Entrada invalida. Saliendo..." << endl;
            break;
        }

        switch (opcion)
        {
        case 1:
            buscarPacientePorDNI();
            break;
        case 2:
            cancelarCita();
            break;
        case 3:
            modificarDisponibilidadMedico();
            break;
        case 4:
            cout << "\n--- LISTA DE CITAS PROGRAMADAS ---" << endl;
            for (const auto& cita : citasProgramadas)
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
        case 5:
            optimizarRutaAmbulancia();
            break;
        case 6:
            gestorRecursos.registrarPacienteCriticoInteractivo();
            break;
        case 7:
            gestorRecursos.asignarRecursosGreedy();
            break;
        case 8:
            gestorRecursos.darAltaPacientesInteractivo();
            break;
        case 9:
            gestorRecursos.reporteRecursos();
            break;
        case 10:
            gestorRecursos.reabastecerMedicamentoInteractivo();
            break;
        case 0:
            cout << "\nSaliendo del Modulo de Gestion de Citas y Recursos." << endl;
            break;
        default:
            cout << "\n[ERROR] Opcion no valida. Intente de nuevo." << endl;
            break;
        }
    } while (opcion != 0);

    return 0;
}
