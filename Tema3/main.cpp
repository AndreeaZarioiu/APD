#include "mpi.h"
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <sstream>
#define NUM_THREADS 4

using namespace std;

pthread_barrier_t barrier;
pthread_mutex_t mutex;

unordered_map<int, string> genre = {
    {0, "horror"}, {1, "comedy"}, {2, "fantasy"}, {3, "science-fiction"}};
unordered_map<string, int> genre_id = {
    {"horror", 0}, {"comedy", 1}, {"fantasy", 2}, {"science-fiction", 3}};

string input = "";
int current = 0;
typedef struct Limits {
  string words;
  string genre;
  int id;
} t_data;
vector<string> order;
int min(double a, double b) { return a < b ? a : b; }
int max(double a, double b) { return a > b ? a : b; }

// function for computing the text
int nr = 0;
int cnt = 0;
int index_of_thread = 0;

void *g(void *arg) {
  t_data s = *(t_data *)arg;
  string p = s.words;
  string str = "aeiouAEIOU";
  int pos = 1;
  int num_words = 0;
  int l = -1;
  bool found = false;
  nr = max(nr, s.id);
  for (int i = 0; i < (int)p.size(); i++) {
    if (p[i] == ' ' || p[i] == '\n') {
      pos = 0;
      if (p[i] == '\n') {
        num_words = 0;
        if (found) {
          reverse(p.begin() + l, p.begin() + i - 1);
          found = false;
        }
        l = -1;
      }
    }
    if (l == -1 && i == 0) l = 0;
    switch (genre_id[s.genre]) {
      case 0:
        if ((str.find(p[i]) == string::npos) &&
            ((p[i] >= 'a' && p[i] <= 'z') || (p[i] >= 'A' && p[i] <= 'Z'))) {
          p.insert(p.begin() + i + 1, tolower(p[i]));
          i++;
        }
        break;
      case 1:
        if (pos % 2 == 0 && p[i] != '\n' && p[i] != ' ') {
          p[i] = toupper(p[i]);
        }
        break;
      case 2:
        if (i == 0 ||
            (i != (int)p.size() - 1 && (p[i - 1] == ' ' || p[i - 1] == '\n'))) {
          p[i] = toupper(p[i]);
        }
        break;
      case 3:
        if (i == 0 || p[i - 1] == ' ') {
          num_words++;
          if (found) {
            reverse(p.begin() + l, p.begin() + i - 1);
            found = false;
          }
          if (num_words == 7) {
            found = true;
            num_words = 0;
            l = i;
          }
        }
    }
    pos++;
  }
  pthread_barrier_wait(&barrier);
  (*(t_data *)arg).words = p;
  pthread_exit(NULL);
}
int num_p = 0;
// function for reading and writing from file
void *f(void *arg) {
  int id = *(int *)arg;
  string line;
  string to_send;
  ifstream myfile(input);
  bool forThisThread = false;
  int count = 0;
  int tag = 0;
  
  if (!myfile.is_open()) {
    cout << "Could not open the input file" << endl;
    return NULL;
  }
  while (getline(myfile, line)) {
    if (!forThisThread && line == genre[id]) {
      forThisThread = true;
    }
    if (line.size() == 0) {
      forThisThread = false;
      int len = to_send.size();
      if (len != 0) {
        to_send = to_string(tag) + " " + to_send;
        to_send = to_string(count) + " " + to_send;
        to_send += "\n";
        num_p++;
        pthread_mutex_lock(&mutex); 
        
        MPI_Send(to_send.c_str(), to_send.size(), MPI_CHAR, id + 1, 1,
                 MPI_COMM_WORLD);

        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &len);
        char *buf = new char[len];
        MPI_Recv(buf, len, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD,
                 &status);
        string msg(buf, len);
        size_t pos = msg.find(" ");
        int index = stoi(msg.substr(0, pos));
       
        if (order.size() < index + 1) {
          order.resize(index + 1);
        }
        msg.erase(0, pos + 1);
        order[index] = msg;
        pthread_mutex_unlock(&mutex); 
       
      }
      tag++;
      count = 0;
      to_send = "";
    }
    if (forThisThread) {
      if (to_send != "" && (tag != 0 || count != 0)) to_send += "\n";
      to_send += line;
      count++;
      line = "";
    }
  }
  pthread_barrier_wait(&barrier);
  if (to_send.size() != 0) {
    int len = to_send.size();
    if (len != 0) {
      to_send = to_string(tag) + " " + to_send;
      to_send = to_string(count) + " " + to_send;
      to_send += "\n";
      num_p++;
      MPI_Send(to_send.c_str(), to_send.size(), MPI_CHAR, id + 1, 1,
               MPI_COMM_WORLD);
      MPI_Status status;
      MPI_Probe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_CHAR, &len);
      char *buf = new char[len];
      MPI_Recv(buf, len, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
      string msg(buf, len);
      size_t pos = msg.find(" ");
      int index = stoi(msg.substr(0, pos));
      if (order.size() < index + 1) {
        order.resize(index + 1);
      }
      msg.erase(0, pos + 1);

      order[index] = msg;
      to_send = "";
    }
  }
 
  to_send = "opreste citire";
  int len = to_send.size();
  MPI_Send(to_send.c_str(), len, MPI_CHAR, id + 1, 1, MPI_COMM_WORLD);
  myfile.close();
  // receive strings
  if (id == 0) {
    string output = input.substr(0, input.size() - 4) + ".out";
    ofstream outfile;
    outfile.open(output, std::ios::app);
    for (int i = 0; i < num_p; i++) {
      outfile << order[i];
    }
    outfile.close();
  }

  pthread_exit(NULL);
}
int masterFunction() {
  pthread_t threads[NUM_THREADS];
  long arguments[NUM_THREADS];
  int r;
  void *status;
  if (pthread_barrier_init(&barrier, NULL, NUM_THREADS)) {
    printf("\n barrier init has failed\n");
    return 1;
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    arguments[i] = i;
    r = pthread_create(&threads[i], NULL, f, &arguments[i]);
    if (r) {
      printf("Eroare la crearea thread-ului %d\n", i);
      return -1;
    }
  }

  for (int id = 0; id < NUM_THREADS; id++) {
    r = pthread_join(threads[id], &status);
    if (r) {
      printf("Eroare la asteptarea thread-ului %d\n", id);
      return -1;
    }
  }
  return 0;
}

void *receiver(void *arg) {
  bool wait = true;
  char *buf1;
  while (wait) {
    int len;
    MPI_Status status1;
    MPI_Probe(0, 1, MPI_COMM_WORLD, &status1);
    MPI_Get_count(&status1, MPI_CHAR, &len);
    buf1 = new char[len];
    MPI_Recv(buf1, len, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status1);
    string recv_char(buf1, len);

    if (recv_char == "opreste citire") {
      wait = false;
    } else {
      // compute P
      size_t pos = recv_char.find(" ");
      int size = stoi(recv_char.substr(0, pos));
      recv_char.erase(0, pos + 1);
      int P = size / 20;
      if (size % 20 != 0) P++;
      if (P > sysconf(_SC_NPROCESSORS_CONF)) P = sysconf(_SC_NPROCESSORS_CONF);

      // compute position
      pos = recv_char.find(" ");
      int num = stoi(recv_char.substr(0, pos));
      string type;
      recv_char.erase(0, pos + 1);

      // compute genre
      pos = recv_char.find("\n");
      type = recv_char.substr(0, pos);
      recv_char.erase(0, pos + 1);
      pthread_t threads[P];
      t_data arguments[P];
      int r;
      void *status;
      if (pthread_barrier_init(&barrier, NULL, P)) {
        printf("\n barrier init has failed\n");
        pthread_exit(NULL);
      }
      stringstream X(recv_char);

      for (int i = 0; i < P; i++) {
        int start = i * (double)size / P;
        int end = min((i + 1) * (double)size / P, size);

        string pgh = "";
        for (int f = 0; f < end - start; f++) {
          string str;
          getline(X, str);
          pgh += str;
          if (str.size() != 0) pgh += " ";
          pgh += "\n";
        }

        arguments[i].words = pgh;
        arguments[i].id = i;
        arguments[i].genre = type;
        r = pthread_create(&threads[i], NULL, g, &arguments[i]);
        if (r) {
          printf("Eroare la crearea thread-ului %d\n", i);
          pthread_exit(NULL);
        }
      }
      string ans = "";
      for (int id = 0; id < P; id++) {
        r = pthread_join(threads[id], &status);
        if (r) {
          printf("Eroare la asteptarea thread-ului %d\n", id);
          pthread_exit(NULL);
        }
      }
      for (int i = 0; i < P; i++) {
        ans = ans + arguments[i].words;
      }
      ans = type + "\n" + ans;
      ans = to_string(num) + " " + ans;
      MPI_Send(ans.c_str(), ans.size(), MPI_CHAR, 0, 1, MPI_COMM_WORLD);
      
    }
    
  }
  delete[] buf1;
  pthread_exit(NULL);
}

int workerFunction() {
  pthread_t thread;
  void *stat;
  int r = pthread_create(&thread, NULL, receiver, NULL);
  if (r) {
    printf("Eroare la crearea thread-ului de receive\n");
    return -1;
  }
  r = pthread_join(thread, &stat);
  if (r) {
    printf("Eroare la asteptarea thread-ului de receive\n");
    return -1;
  }
  return 0;
}
int main(int argc, char *argv[]) {
  int numtasks, rank;
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc == 2) {
    input = argv[1];
  } else {
    cout << "The number of arguments is wrong" << endl;
  }
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    printf("\n mutex init has failed\n");
    return 1;
  }
  if (rank == 0) {
    masterFunction();
  } else {
    workerFunction();
  }

  if (rank == 0) {
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
  }
  MPI_Finalize();
  return 0;
}
