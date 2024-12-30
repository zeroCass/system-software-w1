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



// Function to remove comments and invalid words after the comment
std::string removeComment(const std::string &line) {
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


// Função que remove espaços desnecessários em uma linha
std::string remover_espacos(const std::string &linha) {
    std::string resultado;
    bool espaco_anterior = false;
    bool ant_is_virgula = true;

    for (size_t i = 0; i < linha.size(); ++i) {
        char c = linha[i];

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
std::string preprocessar_linha(const std::string &linha) {
    std::string new_line = linha;
    new_line = removeComment(new_line);
    new_line = remover_espacos(new_line);
    
    

    return new_line;
}


// std::string analisar_linha(const std::string &linha) {
//     std::string linha_completa;
//     bool primeira_instrucao = true;
//     std::istringstream iss(linha);
//     std::string palavra;
//     bool linha_valida = false;
    
//     // Verificar se a linha começa com uma label
//     if (linha.find(':') != std::string::npos) {
//         // Se for label, deve ser associada à próxima instrução
//         linha_completa += linha + " ";  // Mantém a label na mesma linha
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
    std::ofstream output_file(output_filename);

    if (!input_file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de entrada!" << std::endl;
        return;
    }

    if (!output_file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de saída!" << std::endl;
        return;
    }

    std::string linha;

    while (std::getline(input_file, linha)) {
        linha = preprocessar_linha(linha);
        if (linha.empty()) continue; // Ignorar linhas vazias
        output_file << linha << std::endl;

    }
    input_file.close();
    output_file.close();
}


int main() {
    // Nome dos arquivos de entrada e saída
    std::string input_filename = "codigo.asm";  // Nome do arquivo de entrada
    std::string output_filename = "codigo_processado.asm";  // Nome do arquivo de saída

    // Processar o código Assembly
    processar_assembly(input_filename, output_filename);

    std::cout << "Pré-processamento concluído. Código processado gerado em " << output_filename << std::endl;

    return 0;
}
