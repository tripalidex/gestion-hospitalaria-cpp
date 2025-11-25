#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <iomanip>
#include <sstream>

using namespace std;

// --- ESTRUCTURAS DE DATOS ---

struct Sintoma
{
    int idSintoma;
    string nombre;
    int severidad; // 1-10 (1: leve, 10: crítico)
};

struct Enfermedad
{
    string codigoICD10;
    string nombre;
    vector<int> sintomasAsociados;
    int prioridad; // 1: urgente, 2: moderado, 3: bajo
    string especialidadRequerida;
};

struct NodoDecision
{
    int id;
    string pregunta;
    string accionClinica;
    int sintomaAsociado;
    NodoDecision *siOpcion;
    NodoDecision *noOpcion;
    bool esHoja;

    NodoDecision(int _id, string _pregunta)
        : id(_id), pregunta(_pregunta), sintomaAsociado(-1),
          siOpcion(nullptr), noOpcion(nullptr), esHoja(false)
    {
    }
};

struct ResultadoDiagnostico
{
    string codigoICD10;
    string nombreEnfermedad;
    double porcentajeCoincidencia;
    int prioridad;
    string especialidad;
    vector<string> sintomasCoincidentes;
};

// --- DATOS GLOBALES ---

unordered_map<int, Sintoma> baseSintomas;
unordered_map<string, Enfermedad> baseICD10;
NodoDecision *arbolDecisionRaiz = nullptr;
map<pair<vector<int>, string>, double> cachePD;

// --- FUNCIONES DE INICIALIZACIÓN ---

void inicializarBaseSintomas()
{
    baseSintomas = {
        {1, {1, "Fiebre alta", 8}},
        {2, {2, "Tos persistente", 6}},
        {3, {3, "Dificultad respiratoria", 9}},
        {4, {4, "Dolor de cabeza", 5}},
        {5, {5, "Fatiga extrema", 7}},
        {6, {6, "Dolor de garganta", 4}},
        {7, {7, "Dolor muscular", 5}},
        {8, {8, "Perdida del olfato", 6}},
        {9, {9, "Nauseas", 5}},
        {10, {10, "Dolor abdominal", 7}},
        {11, {11, "Diarrea", 6}},
        {12, {12, "Vomitos", 7}},
        {13, {13, "Erupcion cutanea", 4}},
        {14, {14, "Dolor toracico", 9}},
        {15, {15, "Mareos", 6}}};
    cout << "[INFO] Base de sintomas inicializada con " << baseSintomas.size() << " sintomas." << endl;
}

void inicializarBaseICD10()
{
    baseICD10 = {
        {"J00", {"J00", "Rinofaringitis aguda (Resfriado comun)", {2, 4, 6, 7}, 3, "Medicina General"}},
        {"J06.9", {"J06.9", "Infeccion respiratoria aguda", {1, 2, 6, 7}, 2, "Neumologia"}},
        {"U07.1", {"U07.1", "COVID-19", {1, 2, 3, 5, 8}, 1, "Infectologia"}},
        {"J18.9", {"J18.9", "Neumonia", {1, 2, 3, 14}, 1, "Neumologia"}},
        {"A09", {"A09", "Gastroenteritis y colitis", {9, 10, 11, 12}, 2, "Gastroenterologia"}},
        {"R50.9", {"R50.9", "Fiebre no especificada", {1, 4, 5}, 2, "Medicina General"}},
        {"I10", {"I10", "Hipertension esencial", {4, 14, 15}, 2, "Cardiologia"}},
        {"J45.9", {"J45.9", "Asma", {2, 3, 14}, 2, "Neumologia"}}};
    cout << "[INFO] Base ICD-10 inicializada con " << baseICD10.size() << " enfermedades." << endl;
}

NodoDecision *construirArbolDecision()
{
    // Nivel 0: Raíz
    NodoDecision *raiz = new NodoDecision(0, "El paciente presenta fiebre alta (>38.5C)?");
    raiz->sintomaAsociado = 1;

    // Nivel 1: Rama SI (Fiebre presente)
    NodoDecision *fiebreSi = new NodoDecision(1, "Presenta dificultad respiratoria severa?");
    fiebreSi->sintomaAsociado = 3;
    raiz->siOpcion = fiebreSi;

    // Nivel 2: Dificultad respiratoria SI
    NodoDecision *respSi = new NodoDecision(2, "Presenta dolor toracico?");
    respSi->sintomaAsociado = 14;
    fiebreSi->siOpcion = respSi;

    NodoDecision *respSi_SI = new NodoDecision(3, "");
    respSi_SI->accionClinica = "URGENTE: Posible Neumonia (J18.9) o problema cardiaco. Hospitalizacion inmediata.";
    respSi_SI->esHoja = true;
    respSi->siOpcion = respSi_SI;

    NodoDecision *respSi_NO = new NodoDecision(4, "");
    respSi_NO->accionClinica = "URGENTE: Evaluar COVID-19 (U07.1) o infeccion respiratoria severa. Pruebas diagnosticas.";
    respSi_NO->esHoja = true;
    respSi->noOpcion = respSi_NO;

    // Nivel 2: Dificultad respiratoria NO
    NodoDecision *respNo = new NodoDecision(5, "Presenta sintomas gastrointestinales (nauseas, diarrea)?");
    respNo->sintomaAsociado = 9;
    fiebreSi->noOpcion = respNo;

    NodoDecision *gastroSi = new NodoDecision(6, "");
    gastroSi->accionClinica = "MODERADO: Posible Gastroenteritis (A09). Hidratacion y observacion.";
    gastroSi->esHoja = true;
    respNo->siOpcion = gastroSi;

    NodoDecision *gastroNo = new NodoDecision(7, "");
    gastroNo->accionClinica = "MODERADO: Fiebre inespecifica (R50.9). Tratamiento sintomatico y seguimiento.";
    gastroNo->esHoja = true;
    respNo->noOpcion = gastroNo;

    // Nivel 1: Rama NO (Sin fiebre)
    NodoDecision *fiebreNo = new NodoDecision(8, "Presenta tos persistente con dificultad respiratoria?");
    fiebreNo->sintomaAsociado = 2;
    raiz->noOpcion = fiebreNo;

    NodoDecision *tosResp = new NodoDecision(9, "");
    tosResp->accionClinica = "MODERADO: Posible Asma (J45.9). Evaluacion pulmonar y espirometria.";
    tosResp->esHoja = true;
    fiebreNo->siOpcion = tosResp;

    NodoDecision *sinResp = new NodoDecision(10, "");
    sinResp->accionClinica = "LEVE: Resfriado comun (J00) o sintomas menores. Tratamiento ambulatorio.";
    sinResp->esHoja = true;
    fiebreNo->noOpcion = sinResp;

    return raiz;
}

// --- PROGRAMACIÓN DINÁMICA: EMPAREJAMIENTO DE SÍNTOMAS ---

/**
 * Calcula la similitud entre síntomas del paciente y una enfermedad usando PD
 * Complejidad: O(n*m) donde n = síntomas paciente, m = síntomas enfermedad
 */
double calcularCoincidenciaPD(const vector<int> &sintomasPaciente, const vector<int> &sintomasEnfermedad)
{
    int n = sintomasPaciente.size();
    int m = sintomasEnfermedad.size();

    if (n == 0 || m == 0)
        return 0.0;

    // Verificar caché
    pair<vector<int>, string> clave;
    clave.first = sintomasPaciente;
    stringstream ss;
    for (int s : sintomasEnfermedad)
        ss << s << ",";
    clave.second = ss.str();

    if (cachePD.find(clave) != cachePD.end())
    {
        return cachePD[clave];
    }

    // Matriz DP: dp[i][j] = score acumulado
    vector<vector<double>> dp(n + 1, vector<double>(m + 1, 0.0));

    // Llenar matriz con programación dinámica
    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= m; j++)
        {
            int sintomaPaciente = sintomasPaciente[i - 1];
            int sintomaEnfermedad = sintomasEnfermedad[j - 1];

            if (sintomaPaciente == sintomaEnfermedad)
            {
                // Coincidencia exacta: sumar peso basado en severidad
                double peso = baseSintomas[sintomaPaciente].severidad / 10.0;
                dp[i][j] = dp[i - 1][j - 1] + peso;
            }
            else
            {
                // No coincide: tomar el mejor score anterior
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Normalizar score (0-100%)
    double maxScore = 0.0;
    for (int s : sintomasEnfermedad)
    {
        maxScore += baseSintomas[s].severidad / 10.0;
    }

    double resultado = (maxScore > 0) ? (dp[n][m] / maxScore) * 100.0 : 0.0;

    // Guardar en caché
    cachePD[clave] = resultado;

    return resultado;
}

// --- RECURRENCIA: NAVEGACIÓN DEL ÁRBOL DE DECISIÓN ---

string navegarArbolDecision(NodoDecision *nodo, const vector<int> &sintomasPaciente, string &rutaDecision)
{
    if (nodo == nullptr)
        return "";

    // Caso base: es una hoja
    if (nodo->esHoja)
    {
        rutaDecision += " -> [HOJA: " + nodo->accionClinica + "]";
        return nodo->accionClinica;
    }

    // Caso recursivo: evaluar síntoma asociado
    bool sintomPresente = false;
    if (nodo->sintomaAsociado != -1)
    {
        sintomPresente = find(sintomasPaciente.begin(), sintomasPaciente.end(),
                              nodo->sintomaAsociado) != sintomasPaciente.end();
    }

    rutaDecision += "\n  -> [Nodo " + to_string(nodo->id) + ": " + nodo->pregunta +
                    (sintomPresente ? " SI]" : " NO]");

    // Recursión según la respuesta
    if (sintomPresente && nodo->siOpcion != nullptr)
    {
        return navegarArbolDecision(nodo->siOpcion, sintomasPaciente, rutaDecision);
    }
    else if (!sintomPresente && nodo->noOpcion != nullptr)
    {
        return navegarArbolDecision(nodo->noOpcion, sintomasPaciente, rutaDecision);
    }

    return "No se pudo determinar una accion clinica.";
}

// --- FUNCIONES PRINCIPALES DEL MÓDULO ---

void realizarDiagnostico()
{
    cout << "\n=========================================================" << endl;
    cout << "  SISTEMA DE DIAGNOSTICO ASISTIDO" << endl;
    cout << "=========================================================" << endl;

    // Mostrar síntomas disponibles
    cout << "\n[PASO 1] SINTOMAS DISPONIBLES:" << endl;
    for (const auto &par : baseSintomas)
    {
        cout << "  [" << par.first << "] " << par.second.nombre
             << " (Severidad: " << par.second.severidad << "/10)" << endl;
    }

    cout << "\nIngrese los IDs de los sintomas del paciente (separados por espacios, -1 para terminar):" << endl;
    cout << "Ejemplo: 1 2 3 -1" << endl;
    cout << "> ";

    vector<int> sintomasPaciente;
    int idSintoma;
    while (cin >> idSintoma && idSintoma != -1)
    {
        if (baseSintomas.find(idSintoma) != baseSintomas.end())
        {
            sintomasPaciente.push_back(idSintoma);
        }
        else
        {
            cout << "  [ADVERTENCIA] Sintoma ID " << idSintoma << " no existe. Ignorando." << endl;
        }
    }

    if (sintomasPaciente.empty())
    {
        cout << "\n[ERROR] Debe ingresar al menos un sintoma." << endl;
        return;
    }

    cout << "\n[INFO] Sintomas registrados: ";
    for (int id : sintomasPaciente)
    {
        cout << baseSintomas[id].nombre << ", ";
    }
    cout << "\b\b " << endl;

    // PASO 2: Comparación con base ICD-10 usando Programación Dinámica
    cout << "\n[PASO 2] COMPARANDO CON BASE ICD-10 (Programacion Dinamica)..." << endl;

    vector<ResultadoDiagnostico> resultados;
    for (const auto &par : baseICD10)
    {
        const Enfermedad &enf = par.second;

        double porcentaje = calcularCoincidenciaPD(sintomasPaciente, enf.sintomasAsociados);

        if (porcentaje > 0)
        {
            ResultadoDiagnostico resultado;
            resultado.codigoICD10 = enf.codigoICD10;
            resultado.nombreEnfermedad = enf.nombre;
            resultado.porcentajeCoincidencia = porcentaje;
            resultado.prioridad = enf.prioridad;
            resultado.especialidad = enf.especialidadRequerida;

            // Identificar síntomas coincidentes
            for (int idSintPac : sintomasPaciente)
            {
                if (find(enf.sintomasAsociados.begin(), enf.sintomasAsociados.end(),
                         idSintPac) != enf.sintomasAsociados.end())
                {
                    resultado.sintomasCoincidentes.push_back(baseSintomas[idSintPac].nombre);
                }
            }

            resultados.push_back(resultado);
        }
    }

    // Ordenar resultados por porcentaje de coincidencia (descendente)
    sort(resultados.begin(), resultados.end(),
         [](const ResultadoDiagnostico &a, const ResultadoDiagnostico &b)
         {
             return a.porcentajeCoincidencia > b.porcentajeCoincidencia;
         });

    // Mostrar top 3 diagnósticos diferenciales
    cout << "\n[DIAGNOSTICOS DIFERENCIALES - TOP 3]" << endl;
    cout << "-----------------------------------------------------" << endl;

    int limite = min(3, (int)resultados.size());
    for (int i = 0; i < limite; i++)
    {
        cout << "\n"
             << (i + 1) << ". " << resultados[i].nombreEnfermedad
             << " (" << resultados[i].codigoICD10 << ")" << endl;
        cout << "   Coincidencia: " << fixed << setprecision(1)
             << resultados[i].porcentajeCoincidencia << "%" << endl;
        cout << "   Prioridad: " << (resultados[i].prioridad == 1 ? "URGENTE" : resultados[i].prioridad == 2 ? "MODERADA"
                                                                                                             : "BAJA")
             << endl;
        cout << "   Especialidad: " << resultados[i].especialidad << endl;
        cout << "   Sintomas coincidentes: ";
        for (const string &s : resultados[i].sintomasCoincidentes)
        {
            cout << s << ", ";
        }
        cout << "\b\b " << endl;
    }

    // PASO 3: Navegar árbol de decisión clínica
    cout << "\n[PASO 3] ARBOL DE DECISION CLINICA (Recurrencia)..." << endl;
    string rutaDecision = "[INICIO]";
    string accionClinica = navegarArbolDecision(arbolDecisionRaiz, sintomasPaciente, rutaDecision);

    cout << "\nRuta de decision:" << rutaDecision << endl;
    cout << "\n[ACCION CLINICA RECOMENDADA]" << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << accionClinica << endl;

    cout << "\n[INFO] Diagnostico completado exitosamente." << endl;
}

void mostrarBaseSintomas()
{
    cout << "\n=========================================================" << endl;
    cout << "  BASE DE SINTOMAS" << endl;
    cout << "=========================================================" << endl;
    for (const auto &par : baseSintomas)
    {
        cout << "  [" << par.first << "] " << par.second.nombre
             << " (Severidad: " << par.second.severidad << "/10)" << endl;
    }
}

void mostrarBaseICD10()
{
    cout << "\n=========================================================" << endl;
    cout << "  BASE ICD-10" << endl;
    cout << "=========================================================" << endl;
    for (const auto &par : baseICD10)
    {
        cout << "  [" << par.first << "] " << par.second.nombre << endl;
        cout << "    Especialidad: " << par.second.especialidadRequerida
             << " | Prioridad: " << par.second.prioridad << endl;
    }
}

void analizarRendimiento()
{
    cout << "\n=========================================================" << endl;
    cout << "  ANALISIS DE RENDIMIENTO ALGORITMICO" << endl;
    cout << "=========================================================" << endl;

    cout << "\n[PROGRAMACION DINAMICA - Emparejamiento de Sintomas]" << endl;
    cout << "  Complejidad temporal: O(n*m)" << endl;
    cout << "  - n = numero de sintomas del paciente" << endl;
    cout << "  - m = numero de sintomas de la enfermedad" << endl;
    cout << "  Uso de memoizacion: SI (cache global)" << endl;
    cout << "  Entradas en cache: " << cachePD.size() << endl;

    cout << "\n[RECURRENCIA - Arbol de Decision Clinica]" << endl;
    cout << "  Complejidad temporal: O(h)" << endl;
    cout << "  - h = altura del arbol (profundidad maxima)" << endl;
    cout << "  Estructura: Arbol binario de decision" << endl;
    cout << "  Navegacion: Busqueda en profundidad (DFS)" << endl;

    cout << "\n[ESTADISTICAS DEL SISTEMA]" << endl;
    cout << "  Total de sintomas en base: " << baseSintomas.size() << endl;
    cout << "  Total de enfermedades ICD-10: " << baseICD10.size() << endl;
}

void mostrarMenu()
{
    cout << "\n=========================================================" << endl;
    cout << "  SISTEMA DE DIAGNOSTICO ASISTIDO" << endl;
    cout << "=========================================================" << endl;
    cout << "1. Realizar Diagnostico (PD + Recurrencia)" << endl;
    cout << "2. Ver Base de Sintomas" << endl;
    cout << "3. Ver Base ICD-10" << endl;
    cout << "4. Analizar Rendimiento Algoritmico" << endl;
    cout << "0. Salir" << endl;
    cout << "Ingrese su opcion: ";
}

// --- FUNCIÓN MAIN ---

int main()
{
    cout << "\n=========================================================" << endl;
    cout << "  INICIALIZANDO SISTEMA DE DIAGNOSTICO ASISTIDO" << endl;
    cout << "=========================================================" << endl;

    // Inicialización
    inicializarBaseSintomas();
    inicializarBaseICD10();
    arbolDecisionRaiz = construirArbolDecision();
    cout << "[INFO] Arbol de decision clinica construido exitosamente." << endl;

    int opcion;
    do
    {
        mostrarMenu();
        if (!(cin >> opcion))
        {
            cout << "\n[ERROR] Entrada invalida. Saliendo..." << endl;
            cin.clear();
            cin.ignore(10000, '\n');
            break;
        }

        switch (opcion)
        {
        case 1:
            realizarDiagnostico();
            break;
        case 2:
            mostrarBaseSintomas();
            break;
        case 3:
            mostrarBaseICD10();
            break;
        case 4:
            analizarRendimiento();
            break;
        case 0:
            cout << "\nSaliendo del Sistema de Diagnostico Asistido." << endl;
            break;
        default:
            cout << "\n[ERROR] Opcion no valida. Intente de nuevo." << endl;
            break;
        }
    } while (opcion != 0);

    return 0;
}