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
const std::unordered_map<std::string, int> tabela_diretivas = {
    {"CONST", 1},
    {"SPACE", 1}
};

std::unordered_map<std::string, int> tabela_simbolos;



// Converte uma string para uppercase
void to_uppercase(std::string &line) {
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
}


// Ordena as sessoes do codigo de tal forma que a SECTION TEXT seja sempre a primeira e SECTION DATA a ultima
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
    
    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + output_filename);


    output_file << "SECTION TEXT\n";
    for (int i = 0; i < section_text.size(); i++) {
        output_file << section_text[i] << "\n";
    }
    output_file << "SECTION DATA\n";
    for (int j = 0; j < section_data.size(); j++) {
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

    std::string new_line = line.substr(0, pos_comentario);;
    return new_line;

}


// Função que remove espaços desnecessários em uma linha
std::string remover_espacos(const std::string &line) {
    std::string resultado;
    bool espaco_anterior = false;
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
            if (!espaco_anterior && !ant_is_virgula) {
                resultado += ' '; // Add a single space
                espaco_anterior = true;
            }
        } else {
            resultado += c;
            espaco_anterior = false;
            ant_is_virgula = false;
        }
        
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
    if (tabela_diretivas.find(word) != tabela_diretivas.end())
        return true;
    return false;
}



int get_instruction_size(const std::string &word) {
    auto instrucao = tabela_operacoes.find(word);
    if (instrucao != tabela_operacoes.end()) {
        return instrucao->second.tam_instrucao; // retorna o tamanho operacao
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


bool is_simbolo_exists(const std::string &word) {
    if (tabela_simbolos.find(word) != tabela_simbolos.end())
        return true;
    return false;
}

int get_simbol_mem_posicao(const std::string &word) {
    auto simbolo = tabela_simbolos.find(word);
    if (simbolo != tabela_simbolos.end()) {
        return simbolo->second; // retorna posicao de memoria
    }
    return -1;
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


void primeira_passagem(const std::string &input_filename) {
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + input_filename);
    std::string line;
    int contador_posicao = 0;
    int contador_linha = 1;
    std::string word;

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }

        std::istringstream stream(line);
        bool operacao_found = false;

        while (stream >> word && !operacao_found) {
            if (word_is_label(word)) {
                std::string formatted_label = word;
                formatted_label.pop_back(); // remove ':'
                if (is_simbolo_exists(formatted_label)) 
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Simbolo " + word + " ja foi definido.");
                tabela_simbolos[formatted_label] = contador_posicao;
            }
            else if (word_is_instruction(word)) {
                int tam_instrucao = get_instruction_size(word);
                contador_posicao += tam_instrucao;
                operacao_found = true;
            }
            else if (word_is_diretiva(word)) {
                std::cout << "Eh uma diretiva (contador_posicao + 1) - working in progress" << std::endl;
                contador_posicao++;
                operacao_found = true;
            }
            else {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacação " + word + " nao identificada.");
            }
        }
        contador_linha++;
        
    }
    input_file.close();
}

// Funcao que verifica se a label não possui erros lexicos
bool is_lexical_valid(const std::string &word) {
    std::regex pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*:$)");
    return std::regex_match(word, pattern);
}

void segunda_passagem(const std::string &input_filename, const std::string &output_filename) {
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + input_filename);
    
    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + output_filename);
    

    std::string line;
    int contador_posicao = 0;
    int contador_linha = 1;
    std::string word;
    

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }
        // replace virgula por espaços para lidar com o COPY
        std::replace(line.begin(), line.end(), ',', ' ');

        std::istringstream stream(line);
        std::vector<std::string> words;
        int opcode = -1;
        int operando_posicao = -1;
        bool has_label = false;
        // Extrai todas as palvras da linha
        while (stream >> word)
            words.push_back(word);

        for (size_t i = 0; i < words.size(); i++) {
            if (word_is_label(words[i])) {
                if (!is_lexical_valid(words[i]))
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Erro lexico " + word);
                has_label = true;
                continue;
            }



            else if (word_is_instruction(words[i])) {
                // verifica a quantidade de operandos de acordo com tam_instrucao
                int tam_instrucao = get_instruction_size(words[i]);
                if (has_label) tam_instrucao++;

                opcode = get_instruction_opcode(words[i]);
                output_file << opcode << " ";
                

                int t = words.size();

                if (tam_instrucao != t)
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);

                int j = i+1; // proxima instrucao
                for (j; j < tam_instrucao; j++) {
                    if (!is_simbolo_exists(words[j])) 
                       throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Simbolo " + words[j] + " indefinido.");
                    
                    operando_posicao = get_simbol_mem_posicao(words[j]);
                    output_file << operando_posicao  << " "; 
                }
                i = j;
            }



        }
    }
    input_file.close();
    output_file.close();
}


// Função que processa o código de Assembly
void processar_assembly(const std::string &input_filename, const std::string &output_filename) {
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + input_filename);
    
    std::string line;
    std::vector<std::string> output_lines;

    while (std::getline(input_file, line)) {
        
        line = preprocessar_linha(line);
        if (is_single_label(line)) line = correct_single_labels(input_file, line);
        if (line.empty()) continue; // Ignorar linhas vazias

        output_lines.push_back(line);
    }
    input_file.close();

    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) 
        throw std::runtime_error("Não foi possivel abrir o arquivo: " + output_filename);
    
    for (int i = 0; i < output_lines.size(); i++) {
        output_file << output_lines[i] << std::endl;
    }
    output_file.close();
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
            processar_assembly(output_filename, output_filename);
            std::cout << "Pré-processamento concluído. Código processado gerado em " << output_filename << std::endl;
        }
        else {
            std::string output_filename = filename + ".obj";  // Nome do arquivo de saída
            primeira_passagem(input_filename);
            segunda_passagem(input_filename, output_filename);
            // std::cout << "Feature ainda nao implementada para o arquivo: " << output_filename << std::endl;
        }
        
    } catch (std:: exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    
    return EXIT_SUCCESS;
}
