#include <loggers/logger.h>
#include <commons/string.h>

extern t_log* logger;

/// ==========================
/// Helpers
/// ==========================

static const char* opcode_to_str(opcode_t op)
{
    switch (op) {
        case INST_SET: return "SET";
        case INST_SUM: return "SUM";
        case INST_SUB: return "SUB";
        case INST_JNZ: return "JNZ";
        case INST_IO:  return "IO";
        case INST_EXIT:return "EXIT";
        default:       return "UNKNOWN";
    }
}

static const char* reg_to_str(reg_id_t r)
{
    switch (r) {
        case REG_AX: return "AX";
        case REG_BX: return "BX";
        case REG_CX: return "CX";
        case REG_DX: return "DX";
        case REG_EAX:return "EAX";
        case REG_EBX:return "EBX";
        case REG_ECX:return "ECX";
        case REG_EDX:return "EDX";
        case REG_SI: return "SI";
        case REG_DI: return "DI";
        case REG_PC: return "PC";
        default:     return "UNK";
    }
}

/// ==========================
/// Ciclo de instrucción
/// ==========================

void log_cpu_inicio_pid(int pid)
{
    log_info(logger, "CPU comienza a ejecutar PID %d", pid);
}

void log_cpu_fetch(int pid, uint32_t pc, const char* instruccion)
{
    log_info(logger,
        "FETCH - PID: %d - PC: %u - Instrucción: %s",
        pid, pc, instruccion);
}

void log_cpu_decode(opcode_t opcode)
{
    log_info(logger,
        "DECODE - Instrucción: %s",
        opcode_to_str(opcode));
}

void log_cpu_execute(opcode_t opcode)
{
    log_info(logger,
        "EXECUTE - Instrucción: %s",
        opcode_to_str(opcode));
}

/// ==========================
/// Instrucciones
/// ==========================

void log_cpu_set(int pid, reg_id_t reg, uint32_t valor)
{
    log_info(logger,
        "PID: %d - SET %s = %u",
        pid, reg_to_str(reg), valor);
}

void log_cpu_sum(int pid, reg_id_t dst, reg_id_t src)
{
    log_info(logger,
        "PID: %d - SUM %s %s",
        pid, reg_to_str(dst), reg_to_str(src));
}

void log_cpu_sub(int pid, reg_id_t dst, reg_id_t src)
{
    log_info(logger,
        "PID: %d - SUB %s %s",
        pid, reg_to_str(dst), reg_to_str(src));
}

void log_cpu_jnz(int pid, uint32_t nuevo_pc)
{
    log_info(logger,
        "PID: %d - JNZ -> PC = %u",
        pid, nuevo_pc);
}

/// ==========================
/// Memoria / MMU
/// ==========================

void log_cpu_mmu_traduccion(int pid, uint32_t dir_logica, uint32_t dir_fisica)
{
    log_info(logger,
        "PID: %d - MMU Traducción - DL: %u -> DF: %u",
        pid, dir_logica, dir_fisica);
}

void log_cpu_page_fault(int pid, uint32_t pagina)
{
    log_info(logger,
        "Page Fault PID: %d - Página: %u",
        pid, pagina);
}

/// ==========================
/// Interrupciones
/// ==========================

void log_cpu_fin_quantum(int pid)
{
    log_info(logger,
        "PID: %d - Desalojado por fin de Quantum",
        pid);
}

void log_cpu_io(int pid, uint32_t tiempo)
{
    log_info(logger,
        "PID: %d - Solicita IO - Tiempo: %u",
        pid, tiempo);
}

void log_cpu_exit(int pid)
{
    log_info(logger,
        "PID: %d - Finaliza ejecución (EXIT)",
        pid);
}

/// ==========================
/// Contexto
/// ==========================

void log_cpu_contexto_enviado(int pid)
{
    log_info(logger,
        "PID: %d - Contexto enviado al Kernel",
        pid);
}
