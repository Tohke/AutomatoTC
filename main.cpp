#include <iostream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

int main(){
    //Abre o arquivo
    ifstream arquivo("automato.json");

    if(!arquivo.is_open()){
        cerr<<"Erro ao abrir o arquivo JSON!"<<endl;
        return 1;
    }

    //Lê o Json
    json automato;
    arquivo>>automato;

    //Initial e Final

    cout<<"Inicial: " << automato["initial"]<<endl;
    cout<<"Final: "<<automato["final"]<<endl;

    return 0;
}
