#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <filesystem>
#include <algorithm>
#include <regex>



struct Instrucao_Info {
    int opcode_num;
    int tam_instrucao;
};

// Definir a tabela de operações (opcode_simbolo, opcode_num, tam_instrucao)
const std::unordered_map<std::string, Instrucao_Info> tabela_operacoes = {
    {"ADD", {1, 2}},
    {"SUB", {2, 2}},
    {"MULT", {3, 2}},
    {"DIV", {4, 2}},
    {"JMP", {5, 2}},
    {"JMPN", {6, 2}},
    {"JMPP", {7, 2}},
    {"JMPZ", {8, 2}},
    {"COPY", {9, 3}},
    {"LOAD", {10, 2}},
    {"STORE", {11, 2}},
    {"INPUT", {12, 2}},
    {"OUTPUT", {13, 2}},
    {"STOP", {14, 1}}
};

// definir a tabela de diretivas (opcode_diretiva, tam_diretiva)
struct Diretiva_Info {
    int tam_diretiva;
    bool in_header;
};

std::unordered_map<std::string, Diretiva_Info> tabela_diretivas = {
    {"CONST", {2, false}},
    {"SPACE", {1, false}}, // normalmente space so tem tamnho 1, mas pode ter 2
    {"BEGIN", {1, true}},
    {"END", {1, true}},
    {"PUBLIC", {2, true}},
    {"EXTERN", {1, true}},

};

// formato: SIMBOLO, VALOR, IS_EXTERN
struct Simbolo_Info {
    int posicao_mem;
    bool is_extern;
};

std::unordered_map<std::string, Simbolo_Info> tabela_simbolos;
std::unordered_map<std::string, std::vector<int>> tabela_uso;
std::vector<std::string> tabela_uso_insertion_order; // gambiarra para printar a tabela de uso na ordem de utilizacao do codigo
// a gambiarra evitar de usar o vetorr, resultando em o(n) para verificar se uma label ja foi inserida

std::string g_ojb_output; // armazena a output string
int g_is_module = false;
std::vector<std::pair<std::string, int>> g_public_labels;
std::vector<std::string> g_external_labels;
std::vector<int>g_relative_table;


// Definicao de variavies para MACROS
struct MNTEntry {
    std::string macro_name;
    int num_param;
    int mdt_index; // poiscao de onde o corpo da macro esta armazenado na MDT
};

struct MDTEntry {
    int line_number;
    std::string instrucao;
};

// Global tables and index
std::vector<MNTEntry> g_MNT; 
std::vector<MDTEntry> g_MDT; 
int g_current_MDT_index = 0;   // Tracks o index atual da MDT


// Helpers Functions

// Funcao para extrair todas as palavras de uma linha
// separa paralavras unidas por ',' e une palavras separadas por '+'
void extract_words_from_line(const std::string& line, std::vector<std::string>& words) {
    std::string processed_line = line;
    std::replace(processed_line.begin(), processed_line.end(), ',', ' '); // separa strings que estao unidas por virgula (COPY args)
    std::regex join_plus_signal(R"(\s*\+\s*)");
    processed_line = std::regex_replace(processed_line, join_plus_signal, "+"); // aritmetica de ponteiros


    std::istringstream stream(processed_line);
    std::string word;
    words.clear(); 

    while (stream >> word)
        words.push_back(word);
}

std::string extract_first_string(const std::string& line) {
    std::istringstream stream(line);
    std::string first_string;
    stream >> first_string;
    return first_string;
}

void throw_error(const std::string &message, int line) {
    throw std::runtime_error("[linha-" + std::to_string(line) + "]" + message);
}

std::ifstream open_input_file(const std::string& filename) {
    std::ifstream input_file(filename);
    if (!input_file.is_open()) 
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filename);
    return input_file;
}

std::ofstream open_output_file(const std::string& filename) {
    std::ofstream output_file(filename);
    if (!output_file.is_open()) 
        throw std::runtime_error("Não foi possível abrir o arquivo: " + filename);
    return output_file;
}

// Converte uma string para uppercase
void to_uppercase(std::string &line) {
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
}

// Funcção que verifica se uma label está sozinha dada uma linha
bool is_single_label(const std::string &line) {
    if (line.empty()) return false;
    
    std::istringstream stream(line);
    std::string word;
    std::vector<std::string> words;
        // Split the line into words
    while (stream >> word) {
        words.push_back(word);
    }

    if (words[0].back() == ':' && words.size() == 1)
        return true;
    return false;
}

// Função que verifica se um palavra é uma label
bool word_is_label(const std::string &word) {
    if (!word.empty() && word.back() == ':')
        return true;
    return false;
}

// Verifica se uma palavra eh uma instrução
bool word_is_instruction(const std::string &word) {
    if (tabela_operacoes.find(word) != tabela_operacoes.end())
        return true;
    return false;
}

bool word_is_diretiva(const std::string &word) {
    if (tabela_diretivas.find(word) != tabela_diretivas.end()) {
        return true;
    }
    return false;
}

bool word_is_diretiva_header(const std::string &word) {
    auto diretiva = tabela_diretivas.find(word);
    if (diretiva != tabela_diretivas.end() && diretiva->second.in_header) {
        return true;
    }
    return false;
}

// Funcao que verifica se uma string eh um numero valido
bool string_is_number(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') i++; // numero negativo
    
    for (; i < s.size(); i++)
        if (!std::isdigit(s[i])) return false;
    return true;
}

// Verifica se uma string eh um hexadecimal valido comecando sempre com 0x ou 0X
bool string_is_hexnumber(const std::string& s) {
    size_t begin = 0;
    if (s[0] == '-') begin++; // numero negativo

    if (s.size() <= 2 || s[begin] != '0' || (s[begin + 1] != 'x' && s[begin + 1] != 'X')) {
        return false;
    }
    size_t i = begin + 2;

    for (; i < s.size(); i++) {
        if (!std::isxdigit(s[i])) {
            return false;
        }
    }
    return true;
}

// Recebe uma string e se for hexadecimal transoforma o 'x' lowercase
std::string format_hexnumber(const std::string& s) {
    if (!string_is_hexnumber(s))
        return s;
    std::string aux = s;
    size_t pos_x = 1;
    if (aux[0] == '-') pos_x++; //num negativo
    aux[pos_x] = 'x';
    return aux;
}

int get_instruction_size(const std::string &word) {
    auto instrucao = tabela_operacoes.find(word);
    if (instrucao != tabela_operacoes.end()) {
        return instrucao->second.tam_instrucao; // retorna o tamanho operacao
    }
    return -1;
}

int get_diretiva_size(const std::string &word) {
    auto instrucao = tabela_diretivas.find(word);
    if (instrucao != tabela_diretivas.end()) {
        return instrucao->second.tam_diretiva; // retorna o tamanho operacao
    }
    return -1;
}

int get_instruction_opcode(const std::string &word) {
    auto instrucao = tabela_operacoes.find(word);
    if (instrucao != tabela_operacoes.end()) {
        return instrucao->second.opcode_num; // retorna o opcode
    }
    return -1;
}

// Separa string e o numero de operandos que seguem este formato: Y+20
// retorna um pair {string, number}. Caso o number = -1, entao nao tem operacao artimetica de ponteiro
std::pair<std::string, int> extrair_operando_aritimetico(const std::string &word) {
    std::regex pattern(R"(^([a-zA-Z0-9_]+)\+([0-9]+)$)");
    std::smatch match;

    if (std::regex_match(word, match, pattern)) {
        std::string formatted_word = match[1]; // string part
        int number_part = std::stoi(match[2]); // numeric part
        return { formatted_word, number_part };
    }

    return { word, -1 };
        
}

// Função que verifica se determinada label(simbolo) existe na tabelad e simbolos
bool is_simbolo_exists(const std::string &word) {
    auto [formatted_word, _] = extrair_operando_aritimetico(word);
    if (tabela_simbolos.find(formatted_word) != tabela_simbolos.end())
        return true;
    return false;
}

bool is_simbolo_extern(const std::string &word) {
    auto simbolo = tabela_simbolos.find(word);
    if (simbolo != tabela_simbolos.end() && simbolo->second.is_extern) {
        return true;
    }
    return false;
}

// função que retorna a posicao para uma label(simbolo) presente na tabela de simbolo
// caso contrario, retorna -1
int get_simbol_mem_posicao(const std::string &word) {
    auto [operando_label, aritimetica] = extrair_operando_aritimetico(word);

    auto simbolo = tabela_simbolos.find(operando_label);
    if (simbolo != tabela_simbolos.end()) {
        int pos_mem = simbolo->second.posicao_mem;
        if (aritimetica != -1) 
            pos_mem += aritimetica; // se existir aritimetica de ponteiro, soma
        return pos_mem;
    }
    return -1;
}

// Funcao que verifica se a label não possui erros lexicos
bool is_lexical_valid(const std::string &word) {
    std::regex pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*:$)");
    return std::regex_match(word, pattern);
}

// Funcao que escreve a string obj para outputfile
void obj_to_outputfile(const std::string& output_filename) {
    std::ofstream output_file = open_output_file(output_filename);
    g_ojb_output.pop_back(); // remove o ultimo espaço
    
    if (g_is_module) {
        // output da tabela de definicao
        for (size_t i = 0; i < g_public_labels.size(); i++) {
            output_file << "D, " << g_public_labels[i].first << " " << g_public_labels[i].second << "\n";
            
        }
       
        // for (const auto& [simbolo_name, array] : tabela_uso) {
        //     for (const auto& val : array) {
        //          output_file << "U, " << simbolo_name + " " << val << std::endl;
        //     }
        // }

        for (const auto& label : tabela_uso_insertion_order) {
            for (const auto& val : tabela_uso[label]) {
                output_file << "U, " << label + " " << val << std::endl;
            }
            std::cout << "\n";
        }


        output_file << "R, ";
        for (size_t i = 0; i < g_relative_table.size(); i++) {
            output_file << g_relative_table[i] << " ";
        }
        output_file << std::endl;

    }
    output_file << g_ojb_output;
    
    output_file.close();
}

// Função que define se o arquivo informado eh um modulo 
void define_is_module(const std::string &input_filename) {
    std::ifstream input_file = open_input_file(input_filename);
    std::string line;
    bool has_begin = false;
    bool has_end = false;
    while (std::getline(input_file, line)) {
        if (std::regex_search(line, std::regex(R"(\bBEGIN\b)"))) has_begin = true;
        if (std::regex_search(line, std::regex(R"(\bEND\b)"))) has_end = true;
    }
    input_file.close();

    if ((has_begin && !has_end) || (!has_begin && has_end)) throw std::runtime_error("BEGIN/END faltando: " + input_filename);
    if (has_begin && has_end) g_is_module = true; // define que eh modulo

    
}

// Recebe a diretiva SPACE validada e o arquivo de output para alocar o spaco
// Retorna a qtd de espaco alocado
int aloca_space(const std::string &s_number) {
    int number = std::stoi(s_number);
        for (int k = 0; k < number; k++)
            g_ojb_output.append("0 ");
    return number;
}

// Verifica se determinada label esta dentro do array de external labels
bool find_external_label(const std::string label) {
    if (std::find(g_external_labels.begin(), g_external_labels.end(), label) != g_external_labels.end())
        return true;
    return false;
    
}




// Others Functions

// Ordena as sessoes do codigo de tal forma que a SECTION TEXT seja sempre a primeira e SECTION DATA a ultima
// Ja converte cada linha para UPPERCASE
void reordenar_sections(const std::string &input_filename, const std::string &output_filename) {
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + input_filename);
    
    
    std::vector<std::string> section_text;
    std::vector<std::string> section_data;

    std::string line;
    bool in_section_text = false;
    bool in_section_data = false;
    bool has_section_text = false;
    bool has_section_data = false;
    bool has_begin = false;
    bool has_end = true;

    // Funcao assume que SECTION TEXT, SECTION DATA, BEGIN e END estarao sozinhos em um linha

    while (std::getline(input_file, line)) {
        // converte para uppercase sempre
        to_uppercase(line);
        if (line == "SECTION TEXT") {
            in_section_text = true;
            has_section_text = true;
            in_section_data = false;
        } else if (line == "SECTION DATA") {
            in_section_data = true;
            has_section_data = true;
            in_section_text = false;
        } else if (std::regex_search(line, std::regex(R"(\bBEGIN\b)"))) {
            has_begin = true;
        }else if (std::regex_match(line, std::regex(R"(\bEND\b)"))) {
            has_end = true;
        } else {
            if (in_section_text)
                section_text.push_back(line);
            if (in_section_data)
                section_data.push_back(line); 
        }
    }
    input_file.close();
    if (!has_section_text) throw std::runtime_error("Não foi possivel localizar SECTION TEXT no arquivo: " + input_filename);
    if (!has_section_data) throw std::runtime_error("Não foi possivel localizar SECTION DATA no arquivo: " + input_filename);
    if ((has_begin && !has_end) || (!has_begin && has_end)) throw std::runtime_error("BEGIN/END faltando: " + input_filename);
    
    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + output_filename);


    output_file << "SECTION TEXT\n";
    for (size_t i = 0; i < section_text.size(); i++) {
        output_file << section_text[i] << "\n";
    }
    output_file << "SECTION DATA\n";
    for (size_t j = 0; j < section_data.size(); j++) {
        output_file << section_data[j] << "\n";
    }
    output_file.close();

}

// Função que remove comentários em qualquer lugar do código, incluindo no meio da linha (entre operações)
// e no começo da linha. Porem, não permite o uso de palavras reservados dentro do comentario
std::string remover_comentarios(const std::string &line) {
    size_t pos_comentario = line.find(';');
    if (pos_comentario == std::string::npos)
        return line;

    std::istringstream iss(line);
    std::string word;
    std::string new_line;
    std::string aux_line = line;


    bool line_without_comm = false;

    while (!line_without_comm) {
        size_t pos_comentario = aux_line.find(';');
        if (pos_comentario != std::string::npos) {
        
            std::string before = aux_line.substr(0, pos_comentario);
            new_line += before;
            std::string after = aux_line.substr(pos_comentario + 1);
            std::istringstream stream(after);

            size_t word_end_pos = 0;

            while (stream >> word) {
                word_end_pos = after.find(word) + word.length();
                if (tabela_operacoes.find(word) != tabela_operacoes.end()) {
                    new_line += " " + word;
                    break;
                }
            }
            // nova linha agora desconsidera a parte analisada
            aux_line = after.substr(word_end_pos);
    
        } else {
            // If no comment, just copy the line to nova_linha
            new_line += aux_line;
            line_without_comm = true;
        }
    }

    return new_line;
}

// Funcao simplificada que remove comentarios do codigo
// comentarios no começo da linha anulam a ela 
// comentarios no fim da linha são removidos
std::string remover_comentarios_simples(std::string &line) {
    size_t pos_comentario = line.find(';');
    if (pos_comentario == std::string::npos)
        return line;
    
    // se o comentario esta no começo, anula linha e retorna vazio
    if (pos_comentario == 0)
        return std::string();

    std::string new_line = line.substr(0, pos_comentario);
    return new_line;

}

// Função que remove espaços desnecessários em uma linha
std::string remover_espacos(const std::string &line) {
    std::string resultado;
    bool ant_is_space = false;
    bool ant_is_virgula = true;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (i == 0 && isspace(c)) continue;
        
        // se tiver espaços entre virgula, exclui
        if (c == ',') {
            resultado += c;
            ant_is_virgula = true;
            continue;
        }

        if (isspace(c)) {
            if (!ant_is_space && !ant_is_virgula) {
                resultado += ' '; // Add a single space
                ant_is_space = true;
            }
            continue;
        }
        // caso base - somente um char qualquer
        resultado += c;
        ant_is_space = false;
        ant_is_virgula = false;
        
    }

    // Remove trailing space, if any
    if (!resultado.empty() && resultado.back() == ' ') {
        resultado.pop_back();
    }

    return resultado;
}

// Função que remove comentários e espaços desnecessários
std::string preprocessar_linha(const std::string &line) {
    if (line.empty()) return std::string();

    std::string new_line = line;
    // converte para uppercase sempre
    // std::transform(new_line.begin(), new_line.end(), new_line.begin(), ::toupper);

    new_line = remover_comentarios_simples(new_line);
    new_line = remover_espacos(new_line);
    
    

    return new_line;
}


// Funcao que verifica se a diretiva possui argumentos
// retorna { has_arg: boolean, in_next_line: boolean }
std::pair<bool, bool> diretiva_has_args(std::vector<std::string> &words, int idx, std::ifstream &input_file) {
    size_t j = idx + 1;
    if (j < words.size()) {
        return {string_is_number(words[j]) || string_is_hexnumber(words[j]), false};
    }


    std::streampos current_position = input_file.tellg();
    std::string line;
    if (std::getline(input_file, line)) {
        input_file.clear();
        input_file.seekg(current_position); // reseta pointer
        return {string_is_number(extract_first_string(line)) || string_is_hexnumber(extract_first_string(line)), true};
    }

    input_file.clear();
    input_file.seekg(current_position);
    return {false, false};
}

// REmove a quebra de linha (permitida) entre labels e instruções
std::string correct_single_labels(std::ifstream &input_file, const std::string &current_line) {
    std::string next_line;
    while (std::getline(input_file, next_line)) {
        if (!next_line.empty()) {
            next_line = preprocessar_linha(next_line);
            return current_line + " " + next_line; // Combine label with the instruction
        }
    }
    return current_line; 
}

// Fucao que extrair operandos da linha ou de outras linhas se necessario
std::vector<std::string> extrair_operandos(std::ifstream& input_file, std::vector<std::string> words, size_t qtd_operandos, int idx_atual, int contador_linha, int& start_line_from) {
    std::vector<std::string> operandos;
    // coleta operandos da linha atual
    size_t j = idx_atual + 1;
    while (j < words.size() && operandos.size() < qtd_operandos) {
        operandos.push_back(words[j]);
        j++;
    }  

    // If there are more operandos to collect, read more lines
    if (operandos.size() < qtd_operandos) {
        std::string line;
        std::string word;
        std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
        if (!std::getline(input_file, line)) {
            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "] Número de operandos insuficiente.");
        }

        std::replace(line.begin(), line.end(), ',', ' '); // Normalize commas to spaces
        std::istringstream next_stream(line);
        start_line_from = 0;
        while (next_stream >> word && operandos.size() < qtd_operandos) {
            operandos.push_back(word);
            start_line_from++;
        }
        input_file.clear(); // Clear the EOF flag if needed
        input_file.seekg(posicao_linha_atual); // reseta ponteiro
    }

    return operandos;
}

// Função que processa a diretiva 
// Retorna Erro ou tamanho da diretiva
int processa_diretiva(std::ifstream& input_file, std::vector<std::string>& words, int idx_atual, int contador_linha, int& start_line_from, int &contador_posicao) {
    int tam_diretiva = get_diretiva_size(words[idx_atual]);
    std::string next_word;
    std::string line;

    if (words[idx_atual] == "CONST") {
        size_t j = idx_atual + 1;
        // mesma linbha
        if (j < words.size()) {
            if (!string_is_number(words[j]) && !string_is_hexnumber(words[j]))
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[idx_atual]);
            g_ojb_output.append(format_hexnumber(words[j]) + " ");
        } else {
            // procura na proxima linha
            std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
            if (!std::getline(input_file, line)) 
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos insuficiente para " + words[idx_atual]);

            next_word = extract_first_string(line);
            if(!string_is_number(next_word) && !string_is_hexnumber(words[j]))
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[idx_atual]);
            
            g_ojb_output.append(format_hexnumber(next_word) + " ");
            input_file.clear(); // Clear the EOF flag if needed
            input_file.seekg(posicao_linha_atual);
            start_line_from = 1;
                
        }
    }
    else if (words[idx_atual] == "SPACE") {
        size_t j = idx_atual + 1;
        if (j <  words.size()) {
            // se proxa palavra eh diretiva, entao tamanho default
            if (word_is_label(words[j]))
                return tam_diretiva;

            if (!string_is_number(words[j]))
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[idx_atual]);
            // memoria tem que pular a qtd de arg alocado -> -1 pois o primeiro end faz parte da alocacao
            contador_posicao += aloca_space(words[j]) - 1; 
            tam_diretiva = 2;
        }
        else {
            // procura na proxima linha
            std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
            if (!std::getline(input_file, line)) {
                g_ojb_output.append("0 ");
            }
            else {
                // isso aqui ta feio demais kk
                next_word = extract_first_string(line);
                if(word_is_label(next_word)) {
                    g_ojb_output.append("0 ");
                }
                else if (string_is_number(next_word)) {
                    // memoria tem que pular a qtd de arg alocado -> -1 pois o primeiro end faz parte da alocacao
                    contador_posicao += aloca_space(next_word) - 1; 
                    start_line_from = 1;
                    tam_diretiva = 2;
                }
                else 
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha++) + "]Operacao invalida" + next_word);
                
            }
            
            input_file.clear(); // Clear the EOF flag if needed
            input_file.seekg(posicao_linha_atual);
        }
    }

    return tam_diretiva;
}

void processa_diretiva_header(std::ifstream& input_file, std::vector<std::string>& words, int idx_atual, int &contador_linha, int &start_line_from) {
    std::string line;
    if (words[idx_atual] == "EXTERN") {
        // assume que label nao esta sozinha na lina e que ja foi incluida na tabela de simbolos
        std::string label = words[idx_atual - 1];
        label.pop_back(); // remove ":"
        g_external_labels.push_back(label);


    }
    else if (words[idx_atual] == "PUBLIC") {
        size_t j = idx_atual + 1;
        if (j <  words.size()) {
            // se proxa palavra eh diretiva, entao tamanho default
            if (word_is_diretiva(words[j]) || word_is_instruction(words[j]))
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacao invalida " + words[idx_atual]);

            g_public_labels.push_back({words[j], -1});
        }
        else {
            // procura na proxima linha
            std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
            if (!std::getline(input_file, line)) {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacao invalida " + words[idx_atual]);
            }

            if (word_is_diretiva(words[j]) || word_is_instruction(words[j]))
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacao invalida " + words[idx_atual]);

            g_public_labels.push_back({words[j], -1});
        }
    }
}

// Define a posicao de memoria para as public labels
void processa_public_labels() {
    for (size_t i = 0; i < g_public_labels.size(); i++) {
        auto simbolo = tabela_simbolos.find(g_public_labels[i].first);
        if (simbolo == tabela_simbolos.end())
            throw std::runtime_error("Simbolo " + g_public_labels[i].first + " indefinido.");
        g_public_labels[i].second = simbolo->second.posicao_mem;
    }
}

// Adiciona uma label externa para a tabela de uso e insere na insertion_order se preciso
void add_to_tabela_uso(const std::string& label, int position) {
    if (tabela_uso.find(label) == tabela_uso.end()) {
        tabela_uso_insertion_order.push_back(label);
    }
    tabela_uso[label].push_back(position);
}

void add_macro(const std::string& name, int num_params, const std::vector<std::string>& body) {
    MNTEntry mntEntry;
    mntEntry.macro_name = name;
    mntEntry.num_param = num_params;
    mntEntry.mdt_index = g_current_MDT_index;
    g_MNT.push_back(mntEntry);

    // Add macro body to MDT
    for (const auto& line : body) {
        MDTEntry mdtEntry;
        mdtEntry.line_number = g_current_MDT_index;
        mdtEntry.instrucao = line;
        g_MDT.push_back(mdtEntry);
        g_current_MDT_index++;
    }

    // Add ENDMACRO to indicate the end of the macro
    MDTEntry endEntry;
    endEntry.line_number = g_current_MDT_index;;  
    endEntry.instrucao = "ENDMACRO";
    g_MDT.push_back(endEntry);
    g_current_MDT_index++;
}




// Principais Funcoes

std::vector<std::string> expandir_macro_call(const std::string &line) {
    std::vector<std::string> macro_body;
    macro_body.clear();

    for (const auto& mnt_entry : g_MNT) {
            std::regex call_regex(mnt_entry.macro_name + R"((\s(\&?\w+)(?:,(\&?\w+))?(?:,(\&?\w+))?(?:,(\&?\w+))?)?)");
            std::smatch call_match;
            if (std::regex_search(line, call_match, call_regex)) {
                int num_args = 0;
                std::vector<std::string> args;

                for (size_t j = 2; j < call_match.size(); ++j) {
                    if (!call_match[j].str().empty()) {
                        args.push_back(call_match[j].str());
                        num_args++;
                    }
                }

                if (num_args != mnt_entry.num_param) {
                    throw std::runtime_error("Numero de Args invalido para MACRO: " + mnt_entry.macro_name);
                }

                for (size_t j = mnt_entry.mdt_index; j < g_MDT.size(); ++j) {
                    if (g_MDT[j].instrucao == "ENDMACRO") break;
                    std::string replaced_line = g_MDT[j].instrucao;
                    for (size_t k = 0; k < args.size(); ++k) {
                        std::regex argRegex("#" + std::to_string(k + 1));
                        replaced_line = std::regex_replace(replaced_line, argRegex, args[k]);
                    }
                    macro_body.push_back(replaced_line);
                }

            }
    }
    return macro_body;
}

void idenifty_macros_def(std::vector<std::string> &file_lines, std::regex macro_regex) {
    bool in_macro_definition = false;
    std::string current_macro_name;
    int num_params = 0;
    std::vector<std::string> macro_body;
    std::vector<std::string> params;
    std::regex special_char("[^0-9a-zA-Z_\\s\\,\\:]+");
    const std::regex with_and_char("^\\&\\w+$");

    for (size_t i = 0; i < file_lines.size(); ++i) {
        // const std::string current_line = std::regex_replace(file_lines[i], special_char, "");
        const std::string current_line = file_lines[i];
        std::smatch match;
        if (std::regex_match(current_line, match, macro_regex)) {
            if (in_macro_definition) {
                throw std::runtime_error("definicao de MACRO aninhada nao permitido.");
            }

            current_macro_name = match[2].str();
            num_params = 0;
            params.clear();
            
            // Identify and validate argments
            for (size_t j = 4; j < match.size(); ++j) {
                std::string param = match[j].str();
                if (!param.empty()) {
                    
                    std::smatch param_match;
                    if (std::regex_match(param, param_match, with_and_char)) {
                        //std::string param_formatted = param.substr(1);
                        //params.push_back(param_formatted);
                        params.push_back(param);
                        num_params++;
                    }
                    else {
                        throw std::runtime_error("[Syntax-Error]Line[" + current_line + "]Passagem de parametro para Macro invalido. Formato aceito: $x, onde x eh o parametro");
                    }    
                }
            }

            if (num_params > 4) {
                throw std::runtime_error("MACRO nao pode ter mais que 4 params.");
            }

            in_macro_definition = true;
            macro_body.clear();
            file_lines.erase(file_lines.begin() + i); // apaga linha
            i--;
            continue;
        }

        if (in_macro_definition) {
            // expandi macros dentro
            std::vector<std::string> macro_call_body = expandir_macro_call(current_line);
            if (macro_call_body.size() > 0) {
                for (size_t body_line = 0; body_line < macro_call_body.size(); body_line++)
                    macro_body.push_back(macro_call_body[body_line]);
            }
            else if (current_line == "ENDMACRO") {
                // Replace arguments in the macro body with #i format
                for (auto& body_line : macro_body) {
                    for (int j = 0; j < num_params; ++j) {
                        std::string replacement = "#" + std::to_string(j + 1);
                        size_t pos = 0;
                        while ((pos = body_line.find(params[j], pos)) != std::string::npos) {
                            body_line.replace(pos, params[j].length(), replacement);
                            pos += replacement.length(); 
                        }
                    }
                }
                add_macro(current_macro_name, num_params, macro_body);
                in_macro_definition = false;
            } else {
                macro_body.push_back(current_line);
            }
            file_lines.erase(file_lines.begin() + i); // apaga linha
            i--;
        }
    }

    // final do arquivo e nao achou fim da macro
    if (in_macro_definition)
        throw std::runtime_error("ENDMACRO nao achado");


}

void expandir_macros(std::vector<std::string> &file_lines, std::regex macro_regex) {
    std::vector<std::string> macro_body;
    size_t i = 0;
    while (i < file_lines.size()) {
        std::string current_line = file_lines[i];
        macro_body.clear();
        macro_body = expandir_macro_call(current_line);
        if (macro_body.size() > 0) {
            file_lines.erase(file_lines.begin() + i);
            for (size_t j = 0;j < macro_body.size(); j++) {
                file_lines.insert(file_lines.begin() + i, macro_body[j]);
                i++;
            }
        } else {
            i++;
        }  
    }
}

void processar_macros(const std::string &input_filename) {
    std::ifstream input_file = open_input_file(input_filename);    
    // std::regex macro_regex(R"(((\w+):\sMACRO(\s(\w+)(?:,(\w+))?(?:,(\w+))?(?:,(\w+))?)?))");
    std::regex macro_regex(R"(((\w+):\sMACRO(\s(\&?\w+)(?:,(\&?\w+))?(?:,(\&?\w+))?(?:,(\&?\w+))?)?))");

    std::vector<std::string> file_lines;
    std::string line;
    while (std::getline(input_file, line)) {
        file_lines.push_back(line);
    }
    input_file.close();

    idenifty_macros_def(file_lines, macro_regex);
    expandir_macros(file_lines, macro_regex);
    
    // write updated file back
    std::ofstream output_file = open_output_file(input_filename);

    for (const auto& updatedLine : file_lines) {
        output_file << updatedLine << "\n";
    }
    output_file.close();
}

void passagem_zero(const std::string &input_filename, const std::string &output_filename) {
    std::ifstream input_file = open_input_file(input_filename);
    
    std::string line;
    std::vector<std::string> output_lines;

    while (std::getline(input_file, line)) {
        
        line = preprocessar_linha(line);
        if (is_single_label(line)) line = correct_single_labels(input_file, line);
        if (line.empty()) continue; // Ignorar linhas vazias

        output_lines.push_back(line);
    }
    input_file.close();

    std::ofstream output_file = open_output_file(output_filename);
    
    for (size_t i = 0; i < output_lines.size(); i++) {
        output_file << output_lines[i] << std::endl;
    }
    output_file.close();
    processar_macros(input_filename);
}

void primeira_passagem(const std::string &input_filename) {
    std::ifstream input_file = open_input_file(input_filename);
    int contador_posicao = 0;
    int contador_linha = 1;
    bool in_header = false;

    if (g_is_module){
        // processa_modulo(input_file, contador_linha, contador_posicao);
        in_header = true;
    } 

    std::string line;
    std::string word;
    int start_line_from = -1;
    

    

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }
   
        
        std::vector<std::string> words;
        extract_words_from_line(line, words);

        
        
        size_t i = 0;
        while (i < words.size()) {
            if (words[i] == "END") {
                i++;
                continue;
            }

            // verifica se alguma palavra foi usada em alguma instrucao anterior
            if (start_line_from != -1) {
                i = start_line_from;
                
            }
            start_line_from = -1;
            if (i >= words.size()) break;


            if (word_is_label(words[i])) {
                std::string formatted_label = words[i];
                formatted_label.pop_back(); // remove ':'
                if (is_simbolo_exists(formatted_label)) 
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Simbolo " + words[i] + " ja foi definido.");
                tabela_simbolos[formatted_label] = { contador_posicao, false };
                i++;
            }
            else if (word_is_instruction(words[i])) {
                int tam_instrucao = get_instruction_size(words[i]);
                int qtd_operandos = tam_instrucao - 1; // considera o simbolo como parte da instrucao
                extrair_operandos(input_file, words, qtd_operandos, i, contador_linha, start_line_from);
                contador_posicao += tam_instrucao;
                i += tam_instrucao;
                in_header = false;
            }
            else if (word_is_diretiva(words[i])) {
                auto simbolo = tabela_diretivas.find(words[i]);
                if (simbolo == tabela_diretivas.end()) {
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Diretiva " + words[i] + " nao identificada.");
                }

                if (in_header && word_is_diretiva_header(words[i]))
                    processa_diretiva_header(input_file, words, i, contador_linha, start_line_from);

                int tam_diretiva = get_diretiva_size(words[i]);
                auto has_args = diretiva_has_args(words, i, input_file);

                // memoria tem que pular a qtd de arg alocado -> -1 pois o primeiro end faz parte da alocacao
                if (simbolo->first == "SPACE" && has_args.first) {
                    contador_posicao = stoi(words[i + 1]) - 1;  
                    tam_diretiva = 2;
                }
                    
                
                // arg esta na proxima linha
                if (has_args.first && has_args.second)
                    start_line_from = 1;

                i += tam_diretiva;
                contador_posicao++;
            }
            else {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacação " + words[i] + " nao identificada.");
            }

            if (in_header) contador_posicao = 0;
            
        }
        
        contador_linha++;
        
        
    }
    input_file.close();

}

void segunda_passagem(const std::string &input_filename, const std::string &output_filename) {
    std::ifstream input_file = open_input_file(input_filename);
   
    std::string line;
    int contador_posicao = 0;
    int contador_linha = 1;
    std::string word;
    int start_line_from = -1;
    bool in_header = false;

    if (g_is_module){
        // processa_modulo(input_file, contador_linha, contador_posicao);
        in_header = true;
    } 

    

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }
        
        std::vector<std::string> words;
        extract_words_from_line(line, words);

        int opcode = -1;
        int operando_posicao_mem = -1;
        

        size_t i = 0;
        while (i < words.size()) {
            if (words[i] == "END") {
                i++;
                continue;
            }

            // verifica se alguma palavra foi usada em alguma instrucao anterior
            if (start_line_from != -1) {
                i = start_line_from;
                
            }
            start_line_from = -1;
            if (i >= words.size()) break;

            

            if (word_is_label(words[i])) {
                if (!is_lexical_valid(words[i]))
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Erro lexico " + word);
                i++;
                continue;
            }
            else if (word_is_instruction(words[i])) {
                in_header = false;
                std::vector<std::string> operandos;

                // verifica a quantidade de operandos de acordo com tam_instrucao
                int tam_instrucao = get_instruction_size(words[i]); 
                int qtd_operandos = tam_instrucao - 1;  // -1 desconsidera o simbolo da instrucao (ex: LOAD)
                
                opcode = get_instruction_opcode(words[i]);
                g_ojb_output.append(std::to_string(opcode) + " ");
                if (g_is_module) g_relative_table.push_back(0); // instrucao eh absutlo

                operandos = extrair_operandos(input_file, words, qtd_operandos, i, contador_linha, start_line_from);
                // valida os operandos
                for (size_t j = 0; j < operandos.size(); j++) {
                    if (word_is_instruction(operandos[j]))
                        throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + operandos[i]);
                    if (!is_simbolo_exists(operandos[j])) 
                       throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Simbolo " + operandos[j] + " indefinido.");
                    

                    // builda tabela de uso
                    auto [operando_label, _] = extrair_operando_aritimetico(operandos[j]);
                    if (g_is_module && find_external_label(operando_label)) {
                        add_to_tabela_uso(operando_label, contador_posicao + 1 + j);
                        // tabela_uso[operandos[j]].push_back(contador_posicao + 1 + j);
                    }

                    if (g_is_module) g_relative_table.push_back(1);


                    operando_posicao_mem = get_simbol_mem_posicao(operandos[j]);
                    g_ojb_output.append(std::to_string(operando_posicao_mem) + " ");
                }

                

                contador_posicao += tam_instrucao;
                i += tam_instrucao;
            }
            else if (word_is_diretiva(words[i])) {
                int tam_diretiva = processa_diretiva(input_file, words, i, contador_linha, start_line_from, contador_posicao);
                // popula tabela de relativos
                if (g_is_module && !in_header) {
                    auto simbolo = tabela_diretivas.find(words[i]);
                    auto has_args = diretiva_has_args(words, i, input_file);
                    int num_spaces_to_alloc = 1;

                    // memoria tem que pular a qtd de arg alocado -> -1 pois o primeiro end faz parte da alocacao
                    if (simbolo->first == "SPACE" && has_args.first) {
                        contador_posicao = stoi(words[i + 1]) - 1;  
                        tam_diretiva = 2;
                        num_spaces_to_alloc = stoi(words[i + 1]);
                    }

                    for (int i  = 0; i < num_spaces_to_alloc; i++)
                        g_relative_table.push_back(0);
                }
                contador_posicao++;
                i += tam_diretiva;
            }
            else {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacação " + words[i] + " nao identificada.");
            }
            if (in_header) contador_posicao = 0;
        }
        contador_linha++;
    }
    input_file.close();
    if (g_is_module) {
        processa_public_labels();
    }
    obj_to_outputfile(output_filename);
}

int main(int argc, char *argv[]) {
    try {
        if (argc != 2)
            throw std::runtime_error("Uso: ./montador <nome_arquivo.asm> | <nome_arquivo.pre>");

        std::string input_filename = argv[1];
        if (!std::filesystem::exists(input_filename))
            throw std::runtime_error("Arquivo nao encontrado: " + input_filename);

        // extrair extensao do arquivo
        std::string filename = input_filename.substr(0, input_filename.find_last_of("."));
        std::string extension = input_filename.substr(input_filename.find_last_of(".") + 1);
        if (extension != "asm" && extension != "pre")
            throw std::runtime_error("Extensao de arquivo invalida: " + extension);
        
        // Pre-processamento
        if (extension == "asm") {
            std::string output_filename = filename + ".pre";  // Nome do arquivo de saída
            reordenar_sections(input_filename, output_filename);
            passagem_zero(output_filename, output_filename);
            std::cout << "Pré-processamento concluído. Código processado gerado em " << output_filename << std::endl;
        }
        else {
            std::string output_filename = filename + ".obj";  // Nome do arquivo de saída
            define_is_module(input_filename);
            primeira_passagem(input_filename);
            segunda_passagem(input_filename, output_filename);
            std::cout << "Processamento concluído. Código processado gerado em " << output_filename << std::endl;
        }
        
    } catch (std:: exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    
    return EXIT_SUCCESS;
}
