#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// AFD: estados explícitos
#define ESTADOS(X)                                                              \
    X(INICIO)                                                                   \
    X(M1) X(M2) X(MOV_MN) X(A1) X(A2) X(ADD_MN)                                 \
    X(S1) X(S2) X(STO_MN) X(E1) X(E2) X(END_MN)                                 \
    X(MOV_SP) X(MOV_RA) X(MOV_RB) X(MOV_REG) X(MOV_REG_SP)                      \
    X(MOV_COMA) X(MOV_COMA_SP) X(MOV_NUM) X(MOV_FIN)                            \
    X(ADD_SP) X(ADD_RA) X(ADD_AL) X(ADD_AL_SP) X(ADD_COMA) X(ADD_COMA_SP)       \
    X(ADD_RB) X(ADD_BL) X(ADD_FIN)                                              \
    X(STO_SP) X(STO_NUM) X(STO_FIN)                                             \
    X(END_FIN)                                                                  \
    X(ESTADO_ERROR)

#define COMO_ENUM(n) n,
#define COMO_NOMBRE(n) #n,
enum Estado { ESTADOS(COMO_ENUM) };
static const char* NOMBRES_ESTADO[] = { ESTADOS(COMO_NOMBRE) };

enum TipoToken { T_NINGUNO, T_MNEMONICO, T_REGISTRO, T_COMA, T_NUMERO };

static bool esEspacio(char c) { return c == ' ' || c == '\t'; }
static bool esDigito(char c)  { return isdigit(static_cast<unsigned char>(c)) != 0; }

// Estados de aceptación del AFD
static bool esAceptacion(Estado s) {
    switch (s) {
        case MOV_NUM: case MOV_FIN:
        case ADD_BL:  case ADD_FIN:
        case STO_NUM: case STO_FIN:
        case END_MN:  case END_FIN:
            return true;
        default:
            return false;
    }
}

// Tipo de lexema que se forma en cada estado
static TipoToken tipoLexema(Estado s) {
    switch (s) {
        case M1: case M2: case MOV_MN: case A1: case A2: case ADD_MN:
        case S1: case S2: case STO_MN: case E1: case E2: case END_MN:
            return T_MNEMONICO;
        case MOV_RA: case MOV_RB: case MOV_REG:
        case ADD_RA: case ADD_AL: case ADD_RB: case ADD_BL:
            return T_REGISTRO;
        case MOV_COMA: case ADD_COMA:
            return T_COMA;
        case MOV_NUM: case STO_NUM:
            return T_NUMERO;
        default:
            return T_NINGUNO;
    }
}

// Función de transición delta(estado, carácter)
static Estado delta(Estado s, char c) {
    const bool sp = esEspacio(c), dg = esDigito(c);
    switch (s) {
        case INICIO:
            if (sp) return INICIO;
            if (c == 'M') return M1;
            if (c == 'A') return A1;
            if (c == 'S') return S1;
            if (c == 'E') return E1;
            break;

        // mnemónicos
        case M1: if (c == 'O') return M2;      break;
        case M2: if (c == 'V') return MOV_MN;  break;
        case A1: if (c == 'D') return A2;      break;
        case A2: if (c == 'D') return ADD_MN;  break;
        case S1: if (c == 'T') return S2;      break;
        case S2: if (c == 'O') return STO_MN;  break;
        case E1: if (c == 'N') return E2;      break;
        case E2: if (c == 'D') return END_MN;  break;

        case MOV_MN: if (sp) return MOV_SP; break;
        case ADD_MN: if (sp) return ADD_SP; break;
        case STO_MN: if (sp) return STO_SP; break;
        case END_MN: if (sp) return END_FIN; break;

        // MOV R, d
        case MOV_SP:
            if (sp) return MOV_SP;
            if (c == 'A') return MOV_RA;
            if (c == 'B') return MOV_RB;
            break;
        case MOV_RA: case MOV_RB:
            if (c == 'L') return MOV_REG;
            break;
        case MOV_REG: case MOV_REG_SP:
            if (sp) return MOV_REG_SP;
            if (c == ',') return MOV_COMA;
            break;
        case MOV_COMA: case MOV_COMA_SP:
            if (sp) return MOV_COMA_SP;
            if (dg) return MOV_NUM;
            break;
        case MOV_NUM:
            if (dg) return MOV_NUM;
            if (sp) return MOV_FIN;
            break;
        case MOV_FIN:
            if (sp) return MOV_FIN;
            break;

        // ADD AL, BL
        case ADD_SP:
            if (sp) return ADD_SP;
            if (c == 'A') return ADD_RA;
            break;
        case ADD_RA:
            if (c == 'L') return ADD_AL;
            break;
        case ADD_AL: case ADD_AL_SP:
            if (sp) return ADD_AL_SP;
            if (c == ',') return ADD_COMA;
            break;
        case ADD_COMA: case ADD_COMA_SP:
            if (sp) return ADD_COMA_SP;
            if (c == 'B') return ADD_RB;
            break;
        case ADD_RB:
            if (c == 'L') return ADD_BL;
            break;
        case ADD_BL: case ADD_FIN:
            if (sp) return ADD_FIN;
            break;

        // STO d
        case STO_SP:
            if (sp) return STO_SP;
            if (dg) return STO_NUM;
            break;
        case STO_NUM:
            if (dg) return STO_NUM;
            if (sp) return STO_FIN;
            break;
        case STO_FIN:
            if (sp) return STO_FIN;
            break;


        case END_FIN:
            if (sp) return END_FIN;
            break;

        default: break;
    }
    return ESTADO_ERROR;
}

// Mensaje de error cuando el AFD no puede avanzar
static string mensajeError(Estado s, char c, bool fin) {
    const string registroInvalido = "registro inválido: solo se admite AL o BL";
    const string numeroInvalido   = "la dirección debe ser un número decimal no negativo (solo dígitos)";
    switch (s) {
        case INICIO:
            if (fin) return "entrada vacía";
            if (islower(static_cast<unsigned char>(c)))
                return "los mnemónicos deben ir en mayúsculas";
            return "mnemónico desconocido: se esperaba MOV, ADD, STO o END";

        case M1: case M2: case A1: case A2: case S1: case S2: case E1: case E2:
            return "mnemónico inválido o desconocido: se esperaba MOV, ADD, STO o END";

        case MOV_MN: case ADD_MN: case STO_MN:
            if (fin) {
                if (s == STO_MN) return "falta la dirección";
                if (s == MOV_MN) return "faltan los operandos (registro y dirección)";
                return "faltan los operandos (AL, BL)";
            }
            return "debe haber al menos un espacio entre el mnemónico y los operandos";

        case MOV_SP:
            return fin ? "falta el registro destino" : registroInvalido;
        case MOV_RA: case MOV_RB:
            return registroInvalido;
        case MOV_REG: case MOV_REG_SP:
            if (fin) return "falta la coma y la dirección";
            if (esDigito(c)) return "falta la coma entre el registro y la dirección";
            return "se esperaba una coma después del registro";
        case MOV_COMA: case MOV_COMA_SP:
            return fin ? "falta la dirección" : numeroInvalido;
        case MOV_NUM: case MOV_FIN:
            return "sobran componentes después de la dirección";

        case ADD_SP: case ADD_RA:
            return fin ? "falta el registro AL" : "ADD requiere AL como primer registro";
        case ADD_AL: case ADD_AL_SP:
            if (fin) return "falta la coma y el registro BL";
            return "se esperaba una coma después de AL";
        case ADD_COMA: case ADD_COMA_SP:
            return fin ? "falta el registro BL" : "ADD requiere BL como segundo registro";
        case ADD_RB:
            return "ADD requiere BL como segundo registro";
        case ADD_BL: case ADD_FIN:
            return "sobran componentes después de BL";

        case STO_SP:
            return fin ? "falta la dirección"
                       : numeroInvalido + "; STO no admite registros";
        case STO_NUM: case STO_FIN:
            return "sobran componentes después de la dirección";

        case END_MN: case END_FIN:
            return "END no admite operandos";

        default:
            return "error de sintaxis";
    }
}

struct Token {
    TipoToken tipo;
    string lexema;
};

static string nombreToken(const Token& t) {
    switch (t.tipo) {
        case T_MNEMONICO: return t.lexema;
        case T_REGISTRO:  return "REGISTRO";
        case T_COMA:      return "COMA";
        case T_NUMERO:    return "NUMERO";
        default:          return "?";
    }
}

struct ResultadoAFD {
    bool valida = false;
    string error;
    vector<Token> tokens;
    vector<Estado> recorrido;
};

// Recorre la entrada carácter por carácter, valida y produce los tokens
static ResultadoAFD ejecutarAFD(const string& entrada) {
    ResultadoAFD r;
    Estado s = INICIO;
    r.recorrido.push_back(s);

    string buffer;
    auto volcar = [&](TipoToken tipo) {
        if (!buffer.empty()) {
            r.tokens.push_back({tipo, buffer});
            buffer.clear();
        }
    };

    for (size_t i = 0; i < entrada.size(); ++i) {
        const char c = entrada[i];
        const Estado sig = delta(s, c);
        if (sig == ESTADO_ERROR) {
            r.error = mensajeError(s, c, false) + " (carácter '" + string(1, c) +
                      "', posición " + to_string(i + 1) + ")";
            r.recorrido.push_back(ESTADO_ERROR);
            return r;
        }
        // Si cambia la clase de lexema, el lexema anterior terminó
        const TipoToken ta = tipoLexema(s), tn = tipoLexema(sig);
        if (ta != T_NINGUNO && ta != tn) volcar(ta);
        if (tn != T_NINGUNO) buffer += c;

        s = sig;
        r.recorrido.push_back(s);
    }

    if (!esAceptacion(s)) {
        r.error = "instrucción incompleta: " + mensajeError(s, '\0', true);
        r.recorrido.push_back(ESTADO_ERROR);
        return r;
    }
    volcar(tipoLexema(s));
    r.valida = true;
    return r;
}

// Objeto instrucción
struct Instruccion {
    string operacion;
    vector<string> registros;
    bool tieneDireccion = false;
    unsigned long long direccion = 0;
};

// Construye el objeto a partir de los tokens ya validados
static bool construirInstruccion(const vector<Token>& tokens, Instruccion& ins, string& error) {
    for (const Token& t : tokens) {
        switch (t.tipo) {
            case T_MNEMONICO: ins.operacion = t.lexema; break;
            case T_REGISTRO:  ins.registros.push_back(t.lexema); break;
            case T_NUMERO:
                if (t.lexema.size() > 18) {
                    error = "la dirección es demasiado grande (máximo 18 dígitos)";
                    return false;
                }
                ins.tieneDireccion = true;
                ins.direccion = stoull(t.lexema);
                break;
            default: break;
        }
    }
    return true;
}

static void imprimirInstruccion(const Instruccion& ins) {
    cout << "{\n";
    cout << "  operacion: \"" << ins.operacion << "\",\n";
    cout << "  registros: [";
    for (size_t i = 0; i < ins.registros.size(); ++i)
        cout << (i ? ", " : "") << "\"" << ins.registros[i] << "\"";
    cout << "],\n";
    cout << "  direccion: ";
    if (ins.tieneDireccion) cout << ins.direccion; else cout << "null";
    cout << "\n}\n";
}

// Máquina de Moore
enum class EstadoMoore {
    PREPARAR_DIRECCION,   // MAR <- direccion
    LEER_MEMORIA,         // MBR <- M[MAR]
    CARGAR_REGISTRO,      // R <- MBR
    SUMAR,                // ACC <- AL + BL
    ACC_A_MBR,            // MBR <- ACC
    ESCRIBIR_MEMORIA,     // M[MAR] <- MBR
    HALT,                 // HALT <- 1
    FIN                   // sin salida
};

class MaquinaMoore {
public:
    explicit MaquinaMoore(const Instruccion& ins) : ins_(ins) {
        // Selección de la ruta de estados según el objeto instrucción
        if      (ins_.operacion == "MOV") actual_ = EstadoMoore::PREPARAR_DIRECCION;
        else if (ins_.operacion == "ADD") actual_ = EstadoMoore::SUMAR;
        else if (ins_.operacion == "STO") actual_ = EstadoMoore::PREPARAR_DIRECCION;
        else                              actual_ = EstadoMoore::HALT;
    }

    bool terminada() const { return actual_ == EstadoMoore::FIN; }

    // Emite la salida del estado actual y avanza al siguiente
    string siguientePaso() {
        const string out = salida(actual_);
        actual_ = siguiente(actual_);
        return out;
    }

private:
    const Instruccion& ins_;
    EstadoMoore actual_;

    // Función de salida
    string salida(EstadoMoore e) const {
        switch (e) {
            case EstadoMoore::PREPARAR_DIRECCION: return "MAR ← " + to_string(ins_.direccion);
            case EstadoMoore::LEER_MEMORIA:       return "MBR ← M[MAR]";
            case EstadoMoore::CARGAR_REGISTRO:    return ins_.registros[0] + " ← MBR";
            case EstadoMoore::SUMAR:              return "ACC ← AL + BL";
            case EstadoMoore::ACC_A_MBR:          return "MBR ← ACC";
            case EstadoMoore::ESCRIBIR_MEMORIA:   return "M[MAR] ← MBR";
            case EstadoMoore::HALT:               return "HALT ← 1";
            case EstadoMoore::FIN:                return "";
        }
        return "";
    }

    // Función de siguiente estado
    EstadoMoore siguiente(EstadoMoore e) const {
        switch (e) {
            case EstadoMoore::PREPARAR_DIRECCION:
                return ins_.operacion == "MOV" ? EstadoMoore::LEER_MEMORIA
                                               : EstadoMoore::ACC_A_MBR;
            case EstadoMoore::LEER_MEMORIA:      return EstadoMoore::CARGAR_REGISTRO;
            case EstadoMoore::ACC_A_MBR:         return EstadoMoore::ESCRIBIR_MEMORIA;
            case EstadoMoore::CARGAR_REGISTRO:
            case EstadoMoore::SUMAR:
            case EstadoMoore::ESCRIBIR_MEMORIA:
            case EstadoMoore::HALT:
            case EstadoMoore::FIN:               return EstadoMoore::FIN;
        }
        return EstadoMoore::FIN;
    }
};

// Programa principal
static string recortarFinales(string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // muestra bien acentos y la flecha ←
#endif
    bool traza = false;
    string entrada;
    bool hayEntrada = false;

    for (int i = 1; i < argc; ++i) {
        const string a = argv[i];
        if (a == "-t" || a == "--traza") { traza = true; continue; }
        if (hayEntrada) entrada += ' ';
        entrada += a;
        hayEntrada = true;
    }
    if (!hayEntrada) {
        cout << "Ingrese una instrucción: ";
        getline(cin, entrada);
        entrada = recortarFinales(entrada);
    }

    cout << "ENTRADA\n" << entrada << "\n\n";

    // Validación con el AFD
    cout << "VALIDACIÓN MEDIANTE AFD\n";
    const ResultadoAFD afd = ejecutarAFD(entrada);

    if (traza) {
        cout << "Recorrido de estados: ";
        for (size_t i = 0; i < afd.recorrido.size(); ++i)
            cout << (i ? " -> " : "") << NOMBRES_ESTADO[afd.recorrido[i]];
        cout << "\n";
    }

    if (!afd.valida) {
        cout << "Instrucción inválida: " << afd.error << ".\n";
        cout << "No se construye el objeto instrucción.\n";
        cout << "No se generan microoperaciones.\n";
        return 1;
    }

    Instruccion ins;
    string errorObjeto;
    if (!construirInstruccion(afd.tokens, ins, errorObjeto)) {
        cout << "Instrucción inválida: " << errorObjeto << ".\n";
        cout << "No se construye el objeto instrucción.\n";
        cout << "No se generan microoperaciones.\n";
        return 1;
    }
    cout << "Instrucción válida.\n\n";

    cout << "TOKENS RECONOCIDOS\n";
    for (const Token& t : afd.tokens)
        cout << nombreToken(t) << "(\"" << t.lexema << "\")\n";
    cout << "\n";

    cout << "COMPONENTES IDENTIFICADOS\n";
    cout << "Mnemónico: " << ins.operacion << "\n";
    if (ins.operacion == "MOV") {
        cout << "Registro destino: " << ins.registros[0] << "\n";
    } else if (ins.operacion == "ADD") {
        cout << "Registros operandos: " << ins.registros[0] << ", " << ins.registros[1] << "\n";
        cout << "Registro resultado: ACC\n";
    }
    if (ins.tieneDireccion) {
        cout << "Dirección de memoria: " << ins.direccion << "\n";
        cout << "Direccionamiento: directo\n";
    }
    cout << "\n";

    // Objeto instrucción
    cout << "OBJETO INSTRUCCIÓN\n";
    imprimirInstruccion(ins);
    cout << "\n";

    // Máquina de Moore
    cout << "MICROOPERACIONES GENERADAS POR MOORE\n";
    MaquinaMoore moore(ins);
    int n = 1;
    while (!moore.terminada())
        cout << n++ << ". " << moore.siguientePaso() << "\n";
    cout << "Generación terminada.\n";
    return 0;
}
