#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <set>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

struct Regra {
    int origem;
    string simbolo;
    int destino;
};

bool carregarAutomato(const string& caminho, int& inicial, vector<int>& finais, vector<Regra>& regras) {
    ifstream entrada(caminho);
    if (!entrada.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << caminho << endl;
        return false;
    }

    json automatoJson;
    entrada >> automatoJson;
    inicial = automatoJson["initial"];
    finais = automatoJson["final"].get<vector<int>>();

    for (const auto& r : automatoJson["transitions"]) {
        Regra regra;
        regra.origem = stoi(r["from"].get<string>());
        regra.destino = stoi(r["to"].get<string>());
        regra.simbolo = r["read"].is_null() ? "" : r["read"].get<string>();
        regras.push_back(regra);
    }
    return true;
}

string classificarAutomato(const vector<Regra>& regras) {
    for (const Regra& r : regras) {
        if (r.simbolo == "") return "AFND_E";
    }
    map<pair<int, string>, set<int>> transicoes;
    for (const Regra& r : regras) {
        transicoes[{r.origem, r.simbolo}].insert(r.destino);
    }
    for (const auto& par : transicoes) {
        if (par.second.size() > 1) return "AFND";
    }
    return "AFD";
}

bool simularAFD(const string& entrada, int atual, const vector<int>& finais, const vector<Regra>& regras) {
    for (char c : entrada) {
        string s(1, c);
        bool encontrou = false;
        for (const Regra& r : regras) {
            if (r.origem == atual && r.simbolo == s) {
                atual = r.destino;
                encontrou = true;
                break;
            }
        }
        if (!encontrou) return false;
    }
    return find(finais.begin(), finais.end(), atual) != finais.end();
}

bool simularAFND(const string& entrada, int inicial, const vector<int>& finais, const vector<Regra>& regras) {
    set<int> ativos = {inicial};
    for (char c : entrada) {
        string s(1, c);
        set<int> proximos;
        for (int estado : ativos) {
            for (const Regra& r : regras) {
                if (r.origem == estado && r.simbolo == s) {
                    proximos.insert(r.destino);
                }
            }
        }
        if (proximos.empty()) return false;
        ativos = proximos;
    }
    for (int estado : ativos) {
        if (find(finais.begin(), finais.end(), estado) != finais.end()) return true;
    }
    return false;
}

set<int> fechoEpsilon(int estado, const vector<Regra>& regras) {
    set<int> fecho = {estado};
    vector<int> pilha = {estado};
    while (!pilha.empty()) {
        int topo = pilha.back();
        pilha.pop_back();
        for (const Regra& r : regras) {
            if (r.origem == topo && r.simbolo == "" && !fecho.count(r.destino)) {
                fecho.insert(r.destino);
                pilha.push_back(r.destino);
            }
        }
    }
    return fecho;
}

bool simularAFND_E(const string& entrada, int inicial, const vector<int>& finais, const vector<Regra>& regras) {
    set<int> ativos = fechoEpsilon(inicial, regras);
    for (char c : entrada) {
        string s(1, c);
        set<int> proximos;
        for (int estado : ativos) {
            for (const Regra& r : regras) {
                if (r.origem == estado && r.simbolo == s) {
                    set<int> fechoDestino = fechoEpsilon(r.destino, regras);
                    proximos.insert(fechoDestino.begin(), fechoDestino.end());
                }
            }
        }
        if (proximos.empty()) return false;
        ativos = proximos;
    }
    for (int estado : ativos) {
        if (find(finais.begin(), finais.end(), estado) != finais.end()) return true;
    }
    return false;
}

void avaliarCSV(const string& entradaCSV, const string& saidaCSV, const string& tipo,
                int inicial, const vector<int>& finais, const vector<Regra>& regras) {
    ifstream entrada(entradaCSV);
    ofstream saida(saidaCSV);
    if (!entrada.is_open() || !saida.is_open()) {
        cerr << "Erro ao abrir os arquivos de entrada ou saída." << endl;
        return;
    }

    string linha;
    while (getline(entrada, linha)) {
        size_t pos = linha.find(';');
        if (pos == string::npos) continue;

        string palavra = linha.substr(0, pos);
        string esperado = linha.substr(pos + 1);

        auto ini = chrono::high_resolution_clock::now();

        bool resultado = false;
        if (tipo == "AFD") resultado = simularAFD(palavra, inicial, finais, regras);
        else if (tipo == "AFND") resultado = simularAFND(palavra, inicial, finais, regras);
        else if (tipo == "AFND_E") resultado = simularAFND_E(palavra, inicial, finais, regras);

        auto fim = chrono::high_resolution_clock::now();
        auto tempo = chrono::duration_cast<chrono::nanoseconds>(fim - ini).count();

        saida << palavra << ';' << esperado << ';' << (resultado ? "1" : "0") << ';' << tempo << endl;
    }

    entrada.close();
    saida.close();
}

int main(int argc, char* argv[]) {
    if (argc != 4) return 1;

    string caminhoJson = argv[1];
    string arquivoTeste = argv[2];
    string resultadoCSV = argv[3];

    int inicial;
    vector<int> finais;
    vector<Regra> regras;

    if (!carregarAutomato(caminhoJson, inicial, finais, regras)) return 1;

    string tipo = classificarAutomato(regras);
    avaliarCSV(arquivoTeste, resultadoCSV, tipo, inicial, finais, regras);

    return 0;
}