#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

class Urna {
    vector<int> votos;

    public:
    Urna(){
        for(int i =0 ; i<7;i++){
            votos.push_back(0);
        }
    }
    
    void votar(int voto){
        votos[voto]++;
    }
    
    int getVotosCandidato(int candidato){
        return votos[candidato];
    }

    vector<int>* getVotos(){
        return &votos;
    }

};

class Cidade {

    vector<Urna*> urnas;
    int populacao;

    public:
    Cidade(){
       for(int i = 0;i<5;i++){
            Urna *aux = new Urna();
            urnas.push_back(aux);
        }
        populacao = rand() % 29001 + 1000;
    }

    int getPopulacao(){
        return populacao;
    }

    vector<Urna*>* getUrnas(){
        return &urnas;
    }

};

class Reino {
    
    vector<Cidade*> cidades;
    
    public:
    Reino(){
        for(int i = 0;i<20;i++){
            Cidade *aux = new Cidade();
            cidades.push_back(aux);
        }
    }

    vector<Cidade*>* getCidades(){
        return &cidades;
    }
};

class Westeros {
    vector<Reino*> reinos;
    vector<int> votosApurados;
    vector<vector<int>> votosApuradosReinos;
    vector<vector<vector<int>>> votosApuradosCidades;
    atomic<int> urnasApuradas;

    mutex mut;
    condition_variable cv;

    public:
    Westeros(){

        urnasApuradas=0;

        for (int i = 0; i < 7; i++) {
            Reino *aux = new Reino();
            reinos.push_back(aux);

            votosApuradosReinos.emplace_back();
            votosApuradosCidades.emplace_back();
            for (int j = 0; j < 20; j++) {
            votosApuradosCidades.at(i).emplace_back();
                for (int k = 0; k < 7; k++){
                    votosApuradosReinos.at(i).push_back(0);  
                    votosApuradosCidades.at(i).at(j).push_back(0);
                }

            }
        }
            
        votosApurados = {0,0,0,0,0,0,0};
        
    }

    void votar(){
        for(int reino=0;reino<7;reino++)
            for(int cidade=0;cidade<20;cidade++)
                for (int urna=0; urna<5; urna++)
                    for(int i=0;i<reinos.at(reino)->getCidades()->at(cidade)->getPopulacao()/5;i++)
                        reinos.at(reino)->getCidades()->at(cidade)->getUrnas()->at(urna)->votar(rand() % 7);
    }

    void apurar(){
        vector<thread> apuracaoReino;

        for(int reino=0;reino<7;reino++){
            vector<Cidade*> *cidades = reinos.at(reino)->getCidades();
            apuracaoReino.push_back(thread ([] (vector<Cidade*> &cidades, int reino, atomic<int> &urnasApuradas, mutex &mut, condition_variable &cv, 
                                            vector<int> &votosApurados, vector<vector<int>> &votosApuradosReinos, 
                                            vector<vector<vector<int>>> &votosApuradosCidades) {
                
                vector<thread> apuracaoCidade;
                for(int cidade=0;cidade<20;cidade++){
                    vector<Urna*> *urnas = cidades.at(cidade)->getUrnas();
                    apuracaoCidade.push_back(thread ([] (vector<Urna*> &urnas, int reino, int cidade, atomic<int> &urnasApuradas,
                                                    mutex &mut, condition_variable& cv, vector<int> &votosApurados, 
                                                    vector<vector<int>> &votosApuradosReinos, 
                                                    vector<vector<vector<int>>> &votosApuradosCidades) {
                        for (int urna=0; urna<5; urna++){
                                vector<int>* votos = urnas.at(urna)->getVotos();
                                for (int candidato = 0; candidato < 7; candidato++) {
                                    mut.lock();
                                    votosApurados.at(candidato)+= votos->at(candidato);
                                    votosApuradosReinos.at(reino).at(candidato)+= votos->at(candidato);
                                    votosApuradosCidades.at(reino).at(cidade).at(candidato) += votos->at(candidato);
                                    mut.unlock();
                                    cv.notify_all();
                                }
                                urnasApuradas++;
                                sleep(rand() % 119 + 2);
                        }
                    },ref(*urnas), reino, cidade, ref(urnasApuradas), ref(mut), ref(cv), ref(votosApurados), ref(votosApuradosReinos), 
                        ref(votosApuradosCidades)));
                }
                for (auto &i : apuracaoCidade)
                    i.join();

            },ref(*cidades), reino, ref(urnasApuradas), ref(mut), ref(cv), ref(votosApurados), ref(votosApuradosReinos), 
                ref(votosApuradosCidades)));
        }
        for (auto &i : apuracaoReino)
            i.join();
        cv.notify_all();
    }

   int parcialReino(int reino){
        int maior = -1;
        int posicao=0;
        for(int i = 0 ; i<7; i++)
            if(votosApuradosReinos.at(reino).at(i) > maior){
                maior = votosApuradosReinos.at(reino).at(i);
                posicao = i;
            }
        return posicao;
    }

    int parcialCidade(int reino, int cidade){
        int maior = -1;
        int posicao= -1;
        for(int i = 0 ; i<7; i++)
            if(votosApuradosCidades.at(reino).at(cidade).at(i) > maior){
                maior = votosApuradosCidades.at(reino).at(cidade).at(i);
                posicao = i;
            }
        return posicao;
    }

    int parcialVencedor(){
        int maior = -1;
        int posicao= -1;
        for(int i = 0 ; i<7; i++)
            if(votosApurados.at(i) > maior){
                maior = votosApurados.at(i);
                posicao = i;
            }
        return posicao;
    }

    void imprimirParcial() {
        vector<thread> impressao;  
        
        while (urnasApuradas<700){
            unique_lock<mutex>ul (mut);
            cv.wait(ul);

            system("clear");

            cout << "\t\t\t\t\tUrnas Apuradas: " << urnasApuradas << "("<< 100*urnasApuradas/700 << "%)\n\n";

            cout << "\t\t\t\t\tClassificação Geral Atual:" << endl;

            for (int i = 0; i < 4; i++){
                cout << "Candidato " << i+1 <<": " << votosApurados.at(i) << "\t";
            }
            cout << endl;
            
            for (int i = 4; i < 7; i++){
                cout << "Candidato " << i+1 <<": " << votosApurados.at(i) << "\t";
            }
            
            cout << "\n\n";

            cout << "Reino " << 1 << ": " << parcialReino(0)+1; 

            for(int i=1; i < 7; i++){
                cout << "\tReino " << i+1 << ": " << parcialReino(i)+1; 
            }
            cout<< endl<<endl;
            
            for(int j=0; j < 20; j++){
                cout << "Cidade " << j+1 << ": " << parcialCidade(0,j)+1;
                for(int i=1; i < 7; i++)
                    cout << "\tCidade " << j+1 << ": " << parcialCidade(i,j)+1;
                cout<< endl;
            }
                    
                ul.unlock();
        }
    }

    void imprimirFinal(){
        system("clear");

        cout << "\t\t\t\t\tUrnas Apuradas: " << urnasApuradas << "("<< 100*urnasApuradas/700 << "%)\n\n";

        cout << "\t\t\t\t\tClassificação Final:\n\n";

        cout << "\t\t\t\t\tCandidato Eleito: " << parcialVencedor()+1 << "\n\n";

        for (int i = 0; i < 4; i++){
            cout << "Candidato " << i+1 <<": " << votosApurados.at(i) << "\t";
        }
        cout << endl;
        
        for (int i = 4; i < 7; i++){
            cout << "Candidato " << i+1 <<": " << votosApurados.at(i) << "\t";
        }
        
        cout << "\n\n";

        cout << "Reino " << 1 << ": " << parcialReino(0)+1; 

        for(int i=1; i < 7; i++){
            cout << "\tReino " << i+1 << ": " << parcialReino(i)+1; 
        }
        cout<< endl<<endl;
        
        for(int j=0; j < 20; j++){
            cout << "Cidade " << j+1 << ": " << parcialCidade(0,j)+1;
            for(int i=1; i < 7; i++)
                cout << "\tCidade " << j+1 << ": " << parcialCidade(i,j)+1;
            cout<< endl;
        }
    }

};


int main  () {
    
    srand (time(NULL));
   
    Westeros westeros;
    
    westeros.votar();

    thread apuracao([](Westeros &westeros){
        westeros.apurar();
    },ref(westeros));

    int qtdMonitores = rand() % 401 + 100;
    vector<thread> monitores;
    
    for (int i = 0; i < qtdMonitores; i++) {
        monitores.push_back(thread([](Westeros &westeros){
            westeros.imprimirParcial();
        },ref(westeros)));
    }

    apuracao.join();

    for (auto &i : monitores)
        i.join();

    westeros.imprimirFinal();

    return 0;
}
