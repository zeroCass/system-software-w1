#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <filesystem>
#include <algorithm>

// Definir a tabela de operações (simbolo, opcode, tam_operacao)
struct Operacao {
    std::string simbolo;
    std::string opcode;
    int tam_operacao;
};

// Tabela de operações em formato CSV
const std::unordered_map<std::string, Operacao> tabela_operacoes = {
    {"ADD", {"ADD", "01", 2}},
    {"DIV", {"DIV", "04", 2}},
    {"JMP", {"JMP", "05", 2}},
    {"JMPP", {"JMPP", "07", 2}},
    {"JMPZ", {"JMPZ", "08", 2}},
    {"COPY", {"COPY", "09", 3}},
    {"LOAD", {"LOAD", "10", 2}},
    {"STORE", {"STORE", "11", 2}},
    {"INPUT", {"INPUT", "12", 2}},
    {"OUTPUT", {"OUTPUT", "13", 2}},
    {"STOP", {"STOP", "14", 1}}
};

// converte uma string para uppercase
void to_uppercase(std::string &line) {
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
}


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

// Funcção que verifica se uma label está sozinha na linha
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


// std::string analisar_linha(const std::string &line) {
//     std::string linha_completa;
//     bool primeira_instrucao = true;
//     std::istringstream iss(linha);
//     std::string palavra;
//     bool linha_valida = false;
    
//     // Verificar se a linha começa com uma label
//     if (line.find(':') != std::string::npos) {
//         // Se for label, deve ser associada à próxima instrução
//         line_completa += line + " ";  // Mantém a label na mesma linha
//         continue;
//     }

//     // Caso contrário, verificar as instruções
//     while (iss >> palavra) {
//         if (tabela_operacoes.find(palavra) != tabela_operacoes.end()) {
//             // Encontramos uma operação válida
//             const Operacao &op = tabela_operacoes.at(palavra);
//             int tam_operacao = op.tam_operacao;
//             linha_completa += palavra + " "; // Adiciona o simbolo da operação
            
//             // Ler os operandos até completar o tamanho da operação
//             while (--tam_operacao > 0 && iss >> palavra) {
//                 linha_completa += palavra + " ";
//             }

//             // Verificar se a linha foi completada corretamente
//             if (tam_operacao == 0) {
//                 linha_valida = true;
//                 break;
//             }
//         }
//     }

//     // Se a linha for válida (tem operação e operandos suficientes), adicione a linha completa
//     if (linha_valida) {
//         // Escrever a linha no arquivo de saída
//         if (primeira_instrucao) {
//             output_file << linha_completa;
//             primeira_instrucao = false;
//         } else {
//             output_file << "\n" << linha_completa;
//         }
//         linha_completa.clear();  // Limpa a linha para a próxima instrução
//     } else {
//         // Se não for válida, ignorar a linha
//         linha_completa.clear();
//     }
// }

// }


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
            std::cout << "Feature ainda nao implementada para o arquivo: " << output_filename << std::endl;
        }
        
    } catch (std:: exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    
    return EXIT_SUCCESS;
}
