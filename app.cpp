#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <signal.h>
#include <chrono>
#include "rtmidi/RtMidi.h"

#define FILE_TYPE ".mm"
#define MAX_MIDI_BYTES 3
#define MIDI_DATA unsigned char
constexpr unsigned int TIME_WAIT {10}; // short delay (ms) after each midi message

#if defined(_WIN32)
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
  #define CLEAR() printf("\e[1;1H\e[2J");
#else 
  #include <unistd.h>  
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
  #define CLEAR() printf("\033[H\033[J");  
#endif
#define TIMESTAMP() std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

bool done{false};
bool chooseMidiPort(RtMidi &rtmidi);
int read_option(const int);
void flush_stdin();
std::string find_file();


static void finish(int ignore){ 
  done = true;   
}

int main(){
  bool run {true};
  int option{};
  unsigned int count {0};
  constexpr unsigned char T{255}; //terminal char
  std::vector<std::vector<MIDI_DATA>> data; 
  
  while (run == true){    
    std::cout << "\nOptions:" << std::endl;
    //std::cout << "\t0: Create virtual MIDI device" << std::endl;
    std::cout << "\t1: Receive MIDI data from a device" << std::endl;
    std::cout << "\t2: Send MIDI data to a device" << std::endl;
    std::cout << "\t3: Load MIDI data from a file" << std::endl;
    std::cout << "\t4: Save current MIDI data to a file" << std::endl;
    std::cout << "\t5: Print data" << std::endl;
    std::cout << "\t6: Exit (default)" << std::endl;
    option = read_option(6);   
    

     // 0 
    /*------------------------------------------------------------------------  
    RtMidiIn midiin{};  
    if (option <= 0) { // Create virtual device
      
      
      midiin.openVirtualPort	(	std::string( "Virtual MIDI Input" )	);

    }
  // 1 ------------------------------------------------------------------------    
    else */if (option <= 1) { // Read Midi from device
      RtMidiIn midiin{};  
        
      midiin.ignoreTypes( false, false, false );// Don't ignore anything
      
      if (chooseMidiPort(midiin) == false){
        std::cerr << "Could not open any input port!" << std::endl;   
        continue;
      }
      count = 0;
      data.clear();
      done = false;
      (void) signal(SIGINT, finish); // Install an interrupt handler function.      
      std::cout << "Reading MIDI data ..." << std::endl;
      std::cout << "(Stop reading with CTRL + c)" << std::endl;
      while (done == false ) {// Periodically check input queue.
        std::vector<MIDI_DATA> message;
        midiin.getMessage( &message );        
        if ( message.size() <= 0 )
          continue;
        data.push_back(message);   
        for (size_t i=0; i< message.size(); i++)      
          std::cout << std::to_string(message[i]) << " ";            
        std::cout << std::endl;   
        count = count + message.size();
      }      
      midiin.closePort();
      std::cout << std::endl;
      std::cout << "Received " << count << " bytes." << std::endl;
    }

  // 2 ------------------------------------------------------------------------
    else if (option == 2) { // Write Midi to device
      RtMidiOut midiout {}; 
      count = 0;
      if (chooseMidiPort(midiout) == false){
        std::cerr << "Could not open any output port!" << std::endl;   
        continue;
      }    
      if (data.size() < 1){
        std::cerr << "No data recorded yet!" << std::endl;   
        continue;
      }    
      for (auto &m : data){
        midiout.sendMessage( &m );
        SLEEP( TIME_WAIT );    
        for (size_t i=0; i< m.size(); i++)      
          std::cout << std::to_string(m[i]) << " ";
        std::cout << std::endl; 
        count = count + m.size();
      }
      midiout.closePort(); 
      std::cout << std::endl;  
      std::cout << "Sent " << count << " bytes." << std::endl; 
    }

  // 3 ------------------------------------------------------------------------
    else if (option == 3) { // Read file
      count = 0;
      const std::string filename {find_file()};
      if (filename.empty() == true){
        //std::cerr << "No .mid file found in directory!" << std::endl; 
        continue;
      }     
      if (std::FILE* f {std::fopen(filename.data(), "rb")}) { 
        data.clear();
        constexpr size_t buffer_length {256};
        MIDI_DATA buffer [buffer_length]; 
        size_t s {};
        size_t p {};
        size_t t {};
        
        std::vector<MIDI_DATA> line;
        while ((s = std::fread(buffer, sizeof buffer[0], buffer_length, f)) > 0){
            p = 0;
            t = s;
            for (size_t i{0}; i<s; i++){
                if(buffer[i] != T) continue;
                while(( buffer[p] == T ) and ( p<i ))  p++;
                if( buffer[p]== T ) continue;                           
                line.insert(line.end(), buffer + p, buffer + i);
                data.push_back(line);
                line.clear();           
                count += i - p; 
                p = i + 1 ;  
            }
        }    
        if (t > p){ //insert rest
            line.insert(line.end(), buffer + p, buffer + t );
            data.push_back(line);
            line.clear();
            count += t - p;
        }
        std::fclose(f);
        std::cout << "Read " << count << " bytes from file \"" << filename << "\"." <<  std::endl;
      } 
      else { 
        std::cerr << "Unable to open file \"" << filename << "\"." << std::endl; 
      } 
    }

  // 4 ------------------------------------------------------------------------
    else if (option == 4) { // Write to file
      count = 0;
      if (data.size() > 0){
        std::time_t t {std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}; 
        std::string filename{std::ctime(&t)};
        filename="test";
        if ((filename.empty() == false) and (filename[filename.length()-1] == '\n'))
          filename.erase(filename.length()-1);
        filename += FILE_TYPE;        

        if (std::FILE* f {std::fopen(filename.data(), "wb")}) {            
          for (auto &d : data){
            do 
              d.push_back(T); 
            while (d.size() < MAX_MIDI_BYTES);   
            const size_t s {std::fwrite(d.data(), sizeof d[0], d.size(), f)};
            count += s;
          }
          std::fclose(f);
        } else 
          std::cerr << "Could not write to \"" << filename << "\"!" << std::endl;  
        data.clear();
        std::cout << "Wrote " << count << " bytes to \"" << filename << "\"." << std::endl;        
        std::cout << "Data cleared!!!" << std::endl;
      } else
        std::cerr << "No data recorded yet!" << std::endl;  
    }
  // 5 ------------------------------------------------------------------------
    else if (option == 5) { // Print data      
      if (data.size() > 0){
       for (auto &d : data){
        for (auto &b : d)  std::cout << std::to_string(b) << " ";            
        std::cout << std::endl; 
       }      
      } else
        std::cerr << "No data recorded yet!" << std::endl;  
    }
  // 6 ------------------------------------------------------------------------
    else break;
  }  
  return 0;
}

int read_option(const int default_value = 0){  
  int result {default_value};
  if ((std::cin.peek() != '\n') and (not(std::cin >> result))) 
    std::cerr << "Invalid input." << std::endl;  
  flush_stdin();
  return result; 
}

void flush_stdin(){
  int c;  while(((c = getchar()) != '\n') and not(c == EOF)); 
}

bool chooseMidiPort(RtMidi &rtmidi){
  std::string keyHit;
  std::string portName;
  unsigned int i{0}; 
  unsigned int nPorts{rtmidi.getPortCount()};

  std::cout << std::endl;
  
  if (nPorts == 0) {    
    std::cerr << "No midi-ports available!" << std::endl;    
    return false;
  }

  if ( nPorts == 1 )
    std::cout << "Opening " << rtmidi.getPortName() << std::endl;
  else {
    std::cout << "Choose a port number: " << std::endl;
    for (i=0; i<nPorts; i++) {
      portName = rtmidi.getPortName(i);
      std::cout << "\tPort " << i  << " : " << portName << std::endl;      
    }    
    std::cin >> i;    
    std::getline(std::cin, keyHit);  // used to clear out stdin
  }    
  if(( i > nPorts ) or (i < 0)) 
    return false;

  rtmidi.openPort( i );
  return true;
}

std::string find_file(){
    std::string dir = "./";
    std::string filename {""};
    std::vector <std::string> files;
    for (const auto & entry : std::filesystem::directory_iterator(dir)){
        filename = entry.path().string();       
        if (filename.find(FILE_TYPE) == std::string::npos)         
          continue;
        files.push_back(filename);        
    }    
    if (files.size() == 1)
      return files[0];
    else if (files.size() < 1){
      std::cerr << "Could not find any "<< FILE_TYPE << "-files." << std::endl;
      return "";
    }      
    else {
      std::cout << "Found " << files.size() << " ." << FILE_TYPE << "-files." << std::endl;
      std::cout << "Please choose one:" << std::endl;
      for (size_t i {0}; i<files.size(); i++)
        std::cout << "\t" << i+1 << ": " << files[i] << std::endl;
      const int f{read_option(-1)-1};
      if ((f<0) or (f>= (int)files.size()))
        return "";
      filename = files[f];
    }    
    return filename;
}