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
    {"CONST", 2},
    {"SPACE", -1} // -1 significa tamanho variavel, para space 1 ou 2
};

std::unordered_map<std::string, int> tabela_simbolos;


void extract_words_from_line(const std::string& line, std::vector<std::string>& words) {
    std::string processed_line = line;
    std::replace(processed_line.begin(), processed_line.end(), ',', ' '); // separa strings que estao unidas por virgula (COPY args)

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

    std::string new_line = line.substr(0, pos_comentario);
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


bool string_is_number(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') i++; // numero negativo
    
    for (; i < s.size(); i++)
        if (!std::isdigit(s[i])) return false;
    return true;
}


// Recebe uma diretiva validada e o arquivo de output
void processa_space(const std::string &s_number, std::ofstream &output_file) {
    int number = std::stoi(s_number);
        for (int k = 0; k < number; k++)
            output_file << "0" << " ";
}

// Funcao que retorna { has_arg: boolean, in_next_line: boolean }
std::pair<bool, bool> diretiva_has_args(std::vector<std::string> &words, int idx, std::ifstream &input_file) {
    int j = idx + 1;
    if (j < words.size()) {
        return {string_is_number(words[j]), false};
    }


    std::streampos current_position = input_file.tellg();
    std::string line;
    if (std::getline(input_file, line)) {
        input_file.clear();
        input_file.seekg(current_position); // reseta pointer
        return {string_is_number(extract_first_string(line)), true};
    }

    input_file.clear();
    input_file.seekg(current_position);
    return {false, false};
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
    int start_line_from = -1;

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }
   
        
        std::vector<std::string> words;
        extract_words_from_line(line, words);
        
        int i = 0;
        while (i < words.size()) {
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
                tabela_simbolos[formatted_label] = contador_posicao;
                i++;
            }
            else if (word_is_instruction(words[i])) {
                int tam_instrucao = get_instruction_size(words[i]);
                int tam_instrucao_atual = 1; // considera o simbolo como parte da instrucao
                size_t j = i + 1;
                while (j < words.size() && tam_instrucao_atual < tam_instrucao) {
                    j++;
                    tam_instrucao_atual++;
                }

                std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
                // procura operandos em outras linhas
                if (tam_instrucao != tam_instrucao_atual) {
                    start_line_from = 0;
                    while (tam_instrucao_atual < tam_instrucao) {
                        if (!std::getline(input_file, line)) {
                            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos insuficiente para " + words[i]);
                        }
                        std::replace(line.begin(), line.end(), ',', ' ');
                        std::istringstream next_stream(line);
                        while (next_stream >> word) {
                            start_line_from++;
                            tam_instrucao_atual++;
                            if (tam_instrucao_atual == tam_instrucao) {
                                input_file.clear(); // Clear the EOF flag if needed
                                input_file.seekg(posicao_linha_atual); // reseta ponteiro do arquivo para linha anterior
                                break;
                            }
                        }                        
                    }

                }


                contador_posicao += tam_instrucao;
                i += tam_instrucao;
            }
            else if (word_is_diretiva(words[i])) {
                auto simbolo = tabela_diretivas.find(words[i]);
                if (simbolo == tabela_diretivas.end()) {
                    throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Diretiva " + words[i] + " nao identificada.");
                }
                int tam_diretiva = 2;
                auto has_args = diretiva_has_args(words, i, input_file);

                if (simbolo->first == "SPACE" && has_args.first)
                    tam_diretiva = 1;
                
                // arg esta na proxima linha
                if (has_args.second)
                    start_line_from = 1;

                i += tam_diretiva;
                contador_posicao++;
            }
            else {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacação " + words[i] + " nao identificada.");
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
    int start_line_from = -1;
    

    while (std::getline(input_file, line)) {
        if (line == "SECTION TEXT" || line == "SECTION DATA") {
            contador_linha++;
            continue;
        }
        
        std::vector<std::string> words;
        extract_words_from_line(line, words);

        int opcode = -1;
        int operando_posicao_mem = -1;
        bool has_label = false;
        

        int i = 0;
        while (i < words.size()) {
            std::string x = words[i];
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
                std::vector<std::string> operandos;

                // verifica a quantidade de operandos de acordo com tam_instrucao
                int tam_instrucao = get_instruction_size(words[i]); 
                int qtd_operandos = tam_instrucao - 1;  // -1 desconsidera o simbolo da instrucao (ex: LOAD)
                
                opcode = get_instruction_opcode(words[i]);
                output_file << opcode << " ";

                // coleta operandos da linha atual
                size_t j = i + 1;
                while (j < words.size() && operandos.size() < qtd_operandos) {
                    operandos.push_back(words[j]);
                    j++;
                }   
                
                int operandos_size = operandos.size();
                std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
                // coleta operandos de proximas linhas
                while (operandos_size < qtd_operandos) {
                    if (!std::getline(input_file, line)) {
                        throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos insuficiente para " + words[i]);
                    }
                    std::replace(line.begin(), line.end(), ',', ' ');

                    std::istringstream next_stream(line);
                    start_line_from = 0;
                    while (next_stream >> word) {
                        operandos.push_back(word);
                        operandos_size++;
                        start_line_from++;
                        if (operandos_size == qtd_operandos) {
                            input_file.clear(); // Clear the EOF flag if needed
                            input_file.seekg(posicao_linha_atual);
                            break;
                        }
                    }                        
                }

                
                

                // valida os operandos
                for (size_t j = 0; j < operandos.size(); j++) {
                    if (word_is_instruction(operandos[j]))
                        throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + operandos[j]);
                    if (!is_simbolo_exists(operandos[j])) 
                       throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Simbolo " + operandos[j] + " indefinido.");

                    operando_posicao_mem = get_simbol_mem_posicao(operandos[j]);
                    output_file << operando_posicao_mem  << " "; 
                }
                i += tam_instrucao;
            }
            else if (word_is_diretiva(words[i])) {
                int tam_diretiva = 1; //padrao 1
                std::string next_word;

                if (words[i] == "CONST") {
                    tam_diretiva = 2;
                    int j = i + 1;
                    // mesma linbha
                    if (j < words.size()) {
                        if (!string_is_number(words[j]))
                            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);
                        output_file << words[j] << " "; 
                    } else if (j == words.size()) {
                        // procura na proxima linha
                        std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
                        if (!std::getline(input_file, line)) 
                            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos insuficiente para " + words[i]);

                        next_word = extract_first_string(line);
                        if(!string_is_number(next_word))
                            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);
                        
                        output_file << next_word << " ";
                        input_file.clear(); // Clear the EOF flag if needed
                        input_file.seekg(posicao_linha_atual);
                        start_line_from = 1;
                          
                    }
                    else {
                        throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);
                    }
                }
                else if (words[i] == "SPACE") {
                    int j = i + 1;
                    if (j <  words.size()) {
                        if (!string_is_number(words[j]))
                            throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);
                        processa_space(words[j], output_file);
                        tam_diretiva = 2;
                    }
                    else if (j == words.size()) {
                        // procura na proxima linha
                        std::streampos posicao_linha_atual = input_file.tellg(); // salva posicao linha atual
                        if (!std::getline(input_file, line)) {
                            tam_diretiva = 1;
                            output_file << "0" << " ";
                        }
                        else {
                            // isso aqui ta feio demais kk
                            next_word = extract_first_string(line);
                            if(word_is_label(next_word)) {
                                tam_diretiva = 1;
                                output_file << "0" << " ";
                            }
                            else if (string_is_number(next_word)) {
                                processa_space(next_word, output_file);
                                start_line_from = 1;
                                tam_diretiva = 2;
                            }
                            else 
                                throw std::runtime_error("[linha-" + std::to_string(contador_linha++) + "]Operacao invalida" + next_word);
                            
                        }
                        
                        input_file.clear(); // Clear the EOF flag if needed
                        input_file.seekg(posicao_linha_atual);
                    } 
                    else {
                        throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Numero de operandos invalido para " + words[i]);
                    }
                    
                }
                i+=tam_diretiva;
            }
            
            else {
                throw std::runtime_error("[linha-" + std::to_string(contador_linha) + "]Operacação " + words[i] + " nao identificada.");
            }

        }
        contador_linha++;
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
