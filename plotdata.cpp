#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "TH2D.h"
#include "TH1D.h"
#include "TFile.h"
#include "TString.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TF1.h"
#include <sstream>
#include <vector>

using namespace std;

//initialize user selection
struct selection{ 
    string arg[7]; //arg[0]: type of 1st constant, 
                   //arg[4]: value of 1st constant, 
                   //arg[1]:y axis(rms or enc or pulse), 
                   //arg[2]:x axis(gain,channel,peaktime,capacitance), 
                   //arg[3]:z axis(eg.gain,peaktime,channel,capacitance)
                   //arg[5]: type of 2nd constant
                   //arg[6]: value of 2nd constant
    int chn_id[6]; //if arg[3]:z axis is selected to be channels, 
                   //it would be messy to plot all 64 channels, 
                   //hence only 5 channels must be chosen
    int arg_int[7]; //all string arguments are converted to integers 
                    //in the function 'stringtonum()' below
    string title; //final generated title after selection is complete
};

//initialize variables and vectors: this structure contains simple variables to hold temporary read values,
//as well as vectors to fill in chosen read values
struct z{
    int peaktime;
    int gain;
    int chan;
    int vmm_id;
    int cap;
    int delta_u;
    double temp;
    double acq_contr;
    double p_dac;
    double p_dac_er;
    double rms1; //rms acq off
    double rms2; //rms acq off w/o osci noise
    double rms3; //rms acq on
    double rms4; //rms acq on w/o osci noise
    double rms1_er; //rms acq off error
    double rms3_er; //rms acq on error
    double enc1; //enc acq off
    double enc2; //enc acq on
    double enc1_er; //enc acq off error
    double enc2_er; //enc acq on error
    double pa; //pulse amplitude
    double pa_er; //p.a. error
    vector<double> vrms1; //vector:rms acq off
    vector<double> vrms2;//rms acq off w/o osci noise
    vector<double> vrms3;//vector:rms acq on
    vector<double> vrms4;//vector:rms acq on w/o osci noise
    vector<double> vrms1_er;//vector:rms acq off error
    vector<double> vrms3_er;//vector:rms acq on error
    vector<double> venc1;//vector:enc acq off
    vector<double> venc2;//vector:enc acq on
    vector<double> venc1_er;//vector:enc acq off error
    vector<double> venc2_er;//vector:enc acq on error
    vector<double> vpa;//vector:pulse amplitude
    vector<double> vpa_er;//vector:p.a. error
    vector<double> vtemp;
    vector<double> vp_dac;
    vector<double> vp_dac_er;
};

selection StringToNum(selection selection){//convert STRING arguments to INT arguments
//contant1 & constant1 value
if(selection.arg[0]=="channel"){
    selection.arg_int[0]=0;
    selection.arg_int[4] = std::stoi(selection.arg[4]);
} else if(selection.arg[0]=="peaktime"){
    selection.arg_int[0]=1;
    selection.arg_int[4] = std::stoi(selection.arg[4]);
} else if(selection.arg[0]=="gain"){
    selection.arg_int[0]=2;
    selection.arg_int[4] = std::stoi(selection.arg[4]);
}
//y axis
if(selection.arg[1]=="rms_acqon"){ 
    selection.arg_int[1]=0;
} else if(selection.arg[1]=="rms_acqoff"){
    selection.arg_int[1]=1;
} else if(selection.arg[1]=="enc_acqon"){
    selection.arg_int[1]=2;
} else if(selection.arg[1]=="enc_acqoff"){
    selection.arg_int[1]=3;
} else if(selection.arg[1]=="enc"){
    selection.arg_int[1]=4;
} else if(selection.arg[1]=="rms"){
    selection.arg_int[1]=5;
} else if(selection.arg[1]=="pulse_ampl"){
    selection.arg_int[1]=6;
} else if(selection.arg[1]=="temperature"){
    selection.arg_int[1]=7;
} else if(selection.arg[1]=="step"){
    selection.arg_int[1]=8;
}
//x axis
if(selection.arg[2]=="channel"){ 
    selection.arg_int[2]=0;
} else if(selection.arg[2]=="peaktime"){
    selection.arg_int[2]=1;
} else if(selection.arg[2]=="gain"){
    selection.arg_int[2]=2;
} else if(selection.arg[2]=="capacitance"){
    selection.arg_int[2]=3;
} else if(selection.arg[2]=="temperature"){
    selection.arg_int[2]=4;
}
//z axis
if(selection.arg[3]=="channel"){
    selection.arg_int[3]=0;
} else if(selection.arg[3]=="peaktime"){
    selection.arg_int[3]=1;
} else if(selection.arg[3]=="gain"){
    selection.arg_int[3]=2;
} else if(selection.arg[3]=="capacitance"){
    selection.arg_int[3]=3;
}
//constant2 & constant2 value
if(selection.arg[5]=="channel"){
    selection.arg_int[5]=0;
    selection.arg_int[6] = std::stoi(selection.arg[6]);
    selection.chn_id[0] = selection.arg_int[6];
} else if(selection.arg[5]=="peaktime"){
    selection.arg_int[5]=1;
    selection.arg_int[6] = std::stoi(selection.arg[6]);
} else if(selection.arg[5]=="gain"){
    selection.arg_int[5]=2;
    selection.arg_int[6] = std::stoi(selection.arg[6]);
}else if (selection.arg[5] == "q") {
    selection.arg_int[5] = -1;
    selection.arg_int[6] = -1;
}
return selection;
}

selection ModifyArgs(selection selection){ //modifies the gain and peaktime ID-values, when they are chosen 
                                           //as constants, to their actual values; used for the title creation
string gn[8] = {"0.5","1","3","4.5","6","9","12","16"};
string pt[4] = {"200","100","50","25"};
if(selection.arg[0]=="gain"){ //if STRING constant1 is gain
    for(int i=0;i<8;i++){    
        if(selection.arg_int[4]==i){ //check INT value of gain-constant1
            selection.arg[4]=gn[i]; //modify STRING value of gain-constant1
        }
    }
} else if(selection.arg[0]=="peaktime"){ //if STRING constant1 is peaktime
    for(int i=0;i<4;i++){    
        if(selection.arg_int[4]==i){ //check INT value of peaktime-constant1
            selection.arg[4]=pt[i]; //modify STRING value of peaktime-constant1
        }
    }
}

if (selection.arg[5] == "gain") { //if STRING constant2 is gain
    for (int i = 0; i < 8; i++) {
        if (selection.arg_int[6] == i) { //check INT value of gain-constant2
            selection.arg[6] = gn[i]; //modify STRING value of gain-constant2
        }
    }
}
else if (selection.arg[5] == "peaktime") { //if STRING constant1 is peaktime
    for (int i = 0; i < 4; i++) {
        if (selection.arg_int[6] == i) { //check INT value of peaktime-constant2
            selection.arg[6] = pt[i]; //modify STRING value of peaktime-constant2
        }
    }
}
return selection;
}

selection PrintSelection(selection selection){ //this function prints the final selection and creates the title
    cout << "You selected:" << endl;
    ostringstream titlestream;
    selection=ModifyArgs(selection);
    if (selection.arg_int[5] < 0) { //arg_int[5]=-1 when the user doesn't choose one, so if no 2nd constant chosen, plot:
                                    //y vs. x for different z's for [constant1] [contant1_value]
        titlestream << selection.arg[1] << " vs. " << selection.arg[2] << " for different " << selection.arg[3] << "s" << 
        " for " << selection.arg[0] << " " << selection.arg[4] << endl;
    }else if (selection.arg_int[5] >= 0 && (selection.arg_int[3]==3 || selection.arg_int[2]==3)){ 
                                    //if a 2nd constant is chosen, but also capacitance is chosen on either z or x axis, plot:
                                    //y vs. x for different z's for [constant1] [constant1_value] and [constant2] [constant2_value] 
        titlestream << selection.arg[1] << " vs. " << selection.arg[2] << " for different " << selection.arg[3] << "s" << 
        " for " << selection.arg[0] << " " << selection.arg[4]<< " and " << selection.arg[5] << " " << selection.arg[6] << endl;
    }else if (selection.arg_int[5] >= 0 && selection.arg_int[3]!=3 && selection.arg_int[2]!=3){
                                    //if a 2nd constant is chosen and capacitance is not chosen for any axis, plot:
                                    //y vs. x for [constant1] [constant1_value] and [constant2] [constant2_value]
        titlestream << selection.arg[1] << " vs. " << selection.arg[2] << " for " << selection.arg[0] << " " << selection.arg[4] << 
        " and " << selection.arg[5] << " " << selection.arg[6] << endl;
    }
    selection.title = titlestream.str();
    cout << selection.title << endl;
    return selection;
}

ostringstream CreateFilename(selection selection){
    ostringstream namestream;
    if(selection.arg_int[0]==0){
        if (selection.arg_int[5] < 0) {
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[0] << selection.arg[4] << "_" << selection.arg[1] << "_vs_" << selection.arg[2] << "_dif_" << selection.arg[3] << "s";
        }
        else if (selection.arg_int[5] >= 0 && (selection.arg_int[3]==3 || selection.arg_int[2]==3)) {
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[0] << selection.arg[4] << "_" << selection.arg[1] << "_vs_" << selection.arg[2] << "_dif_" << selection.arg[3] << "s_" << selection.arg[5] << "_" << selection.arg[6];
        }
        else if (selection.arg_int[5] >= 0 && selection.arg_int[3]!=3  && selection.arg_int[2]!=3){
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[0] << selection.arg[4] << "_" << selection.arg[1] << "_vs_" << selection.arg[2] << "_" << selection.arg[5] << "_" << selection.arg[6];
        }
        
    } else{
        if (selection.arg_int[5] < 0) {
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[1] << "_vs_" << selection.arg[2] << "_dif_" << selection.arg[3] << "s" << "_" << selection.arg[0] << "_" << selection.arg[4];
        }
        else if (selection.arg_int[5] >= 0 && (selection.arg_int[3]==3 || selection.arg_int[2]==3)) {
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[1] << "_vs_" << selection.arg[2] << "_dif_" << selection.arg[3] << "s" << "_" << selection.arg[0] << "_" << selection.arg[4] << "_" << selection.arg[5] << "_" << selection.arg[6];
        }
        else if (selection.arg_int[5] >= 0 && selection.arg_int[3]!=3 && selection.arg_int[2]!=3){
            namestream << "/media/emorfili/DATA1/HISKP/macros/osci_script/last_result/" << selection.arg[1] << "_vs_" << selection.arg[2] << "_" << selection.arg[0] << "_" << selection.arg[4] << "_" << selection.arg[5] << "_" << selection.arg[6];
        }
    }
return namestream;
}

vector<z> FillVectors(selection slct1, z read, vector<z> z1){ //Fill chosen values to vectors. This function takes as arguments:
                                                              //1: the user selection, 2: a type z read obj which holds temporary read values,
                                                              //3: a type z vector obj to separate between graphs on the z axis 
                                                              //and fill corresponding values. 
switch(slct1.arg_int[0]){ //select constant1 type 
    case 0:  // constant1 = "channel"
        if(read.chan==slct1.arg_int[4]){ //if the current read channel is equal to the value chosen for constant1
            switch(slct1.arg_int[3]){ //select z axis (in case no z axis is chosen, this argument takes the 2nd constant type
                case 1: //z axis(or 2nd const.): peaktime
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have gain or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if gain is on x axis:
                        //CHOICE:: Y: ANY, X: GAIN, Z(OR 2ND CONST.): PEAKTIME, CONSTANT1: CHANNEL                     
                        z1[read.peaktime].peaktime = read.peaktime; //each z axis obj corresponds to a peaktime of certain value{0,1,2,3}
                                                                    //this value is saved to be used later in the legend                          
                        //save current chosen read values to vectors inside z1 vector obj.
                        z1[read.peaktime].venc1.push_back(read.enc1);
                        z1[read.peaktime].venc2.push_back(read.enc2);
                        z1[read.peaktime].venc1_er.push_back(read.enc1_er);
                        z1[read.peaktime].venc2_er.push_back(read.enc2_er);
                        z1[read.peaktime].vrms1.push_back(read.rms1);
                        z1[read.peaktime].vrms2.push_back(read.rms2);
                        z1[read.peaktime].vrms3.push_back(read.rms3);
                        z1[read.peaktime].vrms4.push_back(read.rms4);
                        z1[read.peaktime].vrms1_er.push_back(read.rms1_er);
                        z1[read.peaktime].vrms3_er.push_back(read.rms3_er);
                        z1[read.peaktime].vpa.push_back(read.pa);
                        z1[read.peaktime].vpa_er.push_back(read.pa_er);
                        z1[read.peaktime].vtemp.push_back(read.temp);
                        z1[read.peaktime].vp_dac.push_back(read.p_dac);
                        z1[read.peaktime].vp_dac_er.push_back(read.p_dac_er);
                        //print the chosen data line: 
                        //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                        //here x axis is gain and each entry is identified by the read gain-id {0,1,2,3,4,5,6,7}.
                        cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" << 
                        read.peaktime << " rms_acqoff:" << z1[read.peaktime].vrms1[read.gain] << " rms acqoff error: " << 
                        z1[read.peaktime].vrms1_er[read.gain] << endl;
                    }else{ // if capacitance is on x axis, the 2nd constant is gain, so we only want data with a specific gain
                        if(read.gain==slct1.arg_int[6]){ //if the current read gain is equal to the value chosen for constant2
                            //CHOICE:: Y: ANY, X: CAPACITANCE, Z: PEAKTIME, CONSTANT1: CHANNEL, CONSTANT2: GAIN
                            z1[read.peaktime].peaktime = read.peaktime; //each z axis obj corresponds to a peaktime of certain value{0,1,2,3}
                                                                        //this value is saved to be used later in the legend 
                            //save current chosen read values to vectors inside z1 vector obj.
                            z1[read.peaktime].venc1.push_back(read.enc1);
                            z1[read.peaktime].venc2.push_back(read.enc2);
                            z1[read.peaktime].venc1_er.push_back(read.enc1_er);
                            z1[read.peaktime].venc2_er.push_back(read.enc2_er);
                            z1[read.peaktime].vrms1.push_back(read.rms1);
                            z1[read.peaktime].vrms2.push_back(read.rms2);
                            z1[read.peaktime].vrms3.push_back(read.rms3);
                            z1[read.peaktime].vrms4.push_back(read.rms4);
                            z1[read.peaktime].vrms1_er.push_back(read.rms1_er);
                            z1[read.peaktime].vrms3_er.push_back(read.rms3_er);
                            z1[read.peaktime].vpa.push_back(read.pa);
                            z1[read.peaktime].vpa_er.push_back(read.pa_er);
                            z1[read.peaktime].vtemp.push_back(read.temp);
                            z1[read.peaktime].vp_dac.push_back(read.p_dac);
                            z1[read.peaktime].vp_dac_er.push_back(read.p_dac_er);
                            //print the chosen data line
                            //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                            //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7,8}.
                            cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                            << read.peaktime << " cap:" << read.cap << " rms_acqoff:" << z1[read.peaktime].vrms1[read.cap] <<
                            " rms acqoff error: " << z1[read.peaktime].vrms1_er[read.cap] << endl;
                        } //check value of constant2 (gain)
                    } //check if capacitance is on x axis
                    break;
                case 2: // z axis(or 2nd const.): gain
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have peaktime or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if peaktime is on x axis:
                        //CHOICE:: Y: ANY, X: PEAKTIME, Z(OR 2ND CONST.): GAIN, CONSTANT1: CHANNEL
                        z1[read.gain].gain = read.gain; //each z axis obj corresponds to a gain of certain value{0,1,2,3,4,5,6,7}
                                                        //this value is saved to be used later in the legend  
                        //save current chosen read values to vectors inside z1 vector obj.
                        z1[read.gain].venc1.push_back(read.enc1);
                        z1[read.gain].venc2.push_back(read.enc2);
                        z1[read.gain].venc1_er.push_back(read.enc1_er);
                        z1[read.gain].venc2_er.push_back(read.enc2_er);
                        z1[read.gain].vrms1.push_back(read.rms1);
                        z1[read.gain].vrms2.push_back(read.rms2);
                        z1[read.gain].vrms3.push_back(read.rms3);
                        z1[read.gain].vrms4.push_back(read.rms4);
                        z1[read.gain].vrms1_er.push_back(read.rms1_er);
                        z1[read.gain].vrms3_er.push_back(read.rms3_er);
                        z1[read.gain].vpa.push_back(read.pa);
                        z1[read.gain].vpa_er.push_back(read.pa_er);
                        z1[read.gain].vtemp.push_back(read.temp);
                        z1[read.gain].vp_dac.push_back(read.p_dac);
                        z1[read.gain].vp_dac_er.push_back(read.p_dac_er);
                        //print the chosen data line
                        //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                        //here x axis is peaktime and each entry is identified by the read peaktime id {0,1,2,3}.
                        cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                        << read.peaktime << " rms_acqoff:" << z1[read.gain].vrms1[read.peaktime] << " rms acqoff error: " 
                        << z1[read.gain].vrms1_er[read.peaktime] << endl;
                    }else{ // if capacitance is on x axis, the 2nd constant is peaktime, so we only want data with a specific peaktime
                        if(read.peaktime==slct1.arg_int[6]){ //if the current read peaktime is equal to the value chosen for constant2
                            //CHOICE:: Y: ANY, X: CAPACITANCE, Z: GAIN, CONSTANT1: CHANNEL, CONSTANT2: PEAKTIME
                            z1[read.gain].gain = read.gain; //each z axis obj corresponds to a gain of certain value{0,1,2,3,4,5,6,7}
                                                            //this value is saved to be used later in the legend 
                            //save current chosen read values to vectors inside z1 vector obj.
                            z1[read.gain].venc1.push_back(read.enc1);
                            z1[read.gain].venc2.push_back(read.enc2);
                            z1[read.gain].venc1_er.push_back(read.enc1_er);
                            z1[read.gain].venc2_er.push_back(read.enc2_er);
                            z1[read.gain].vrms1.push_back(read.rms1);
                            z1[read.gain].vrms2.push_back(read.rms2);
                            z1[read.gain].vrms3.push_back(read.rms3);
                            z1[read.gain].vrms4.push_back(read.rms4);
                            z1[read.gain].vrms1_er.push_back(read.rms1_er);
                            z1[read.gain].vrms3_er.push_back(read.rms3_er);
                            z1[read.gain].vpa.push_back(read.pa);
                            z1[read.gain].vpa_er.push_back(read.pa_er);
                            z1[read.gain].vtemp.push_back(read.temp);
                            z1[read.gain].vp_dac.push_back(read.p_dac);
                            z1[read.gain].vp_dac_er.push_back(read.p_dac_er);
                            //print the chosen data line
                            //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                            //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7,8}.
                            cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                            << read.peaktime << " cap:" << read.cap << " rms_acqoff:" << z1[read.gain].vrms1[read.cap] <<
                            " rms acqoff error: " << z1[read.gain].vrms1_er[read.cap] << endl;  
                        } //check value of constant2 (peaktime)
                    } //check if capacitance is on x axis
                    break;
                case 3: //z axis: capacitance: in this case x axis is definitely  
                        //not capacitance, but also again a 2nd constant exists
                    switch(slct1.arg_int[5]){ //check constant2 type
                        case 1: //constant2: peaktime
                            if(read.peaktime==slct1.arg_int[6]){ //if the current read peaktime is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: GAIN, Z: CAPACITANCE, CONSTANT1: CHANNEL, CONSTANT2: PEAKTIME
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend 
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);
                                z1[read.cap].vtemp.push_back(read.temp);
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is gain and each entry is identified by the read gain-id {0,1,2,3,4,5,6,7}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" rms_acqoff:" << z1[read.cap].vrms1[read.gain] <<
                                " rms acqoff error: " << z1[read.cap].vrms1_er[read.gain] << endl;                                            
                            }
                            break;
                        case 2: //constant2: gain
                            if(read.gain==slct1.arg_int[6]){ //if the current read gain is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: PEAKTIME, Z: CAPACITANCE, CONSTANT1: CHANNEL, CONSTANT2: GAIN
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);
                                z1[read.cap].vtemp.push_back(read.temp);
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is peaktime and each entry is identified by the read peaktime-id {0,1,2,3}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" rms_acqoff:" << z1[read.cap].vrms1[read.peaktime] <<
                                " rms acqoff error: " << z1[read.cap].vrms1_er[read.peaktime] << endl;                                            
                            } //check value of constant2
                            break; 
                    } //switch constant2
                    break;
            } //switch z axis
        } //check value of constant1 (channel)
        break;
    case 1:  //constant1 = "peaktime"
        if(read.peaktime==slct1.arg_int[4]){ //if the current read peaktime is equal to the value chosen for constant1
            switch(slct1.arg_int[3]){ //select z axis (in case no z axis is chosen and z!=capacitance, this argument takes the 2nd constant type
                case 0: //z axis(or 2nd const.): channel
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have gain or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if gain is on x axis:
                        for(int k=0;k<6;k++){ 
                            if(read.chan==slct1.chn_id[k]){ //when channel is on z axis, only 5 chosen channels are plotted,
                                                            //here, we loop through all chosen channels, and check whether the
                                                            //current read channel is one of them
                                //CHOICE:: Y: ANY, X: GAIN, Z(OR 2ND CONST.): CHANNEL, CONSTANT1: PEAKTIME                        
                                z1[k].chan = read.chan; //each z axis obj corresponds to a channel of certain value{0-63} 
                                                        //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[k].venc1.push_back(read.enc1);
                                z1[k].venc2.push_back(read.enc2);
                                z1[k].venc1_er.push_back(read.enc1_er);
                                z1[k].venc2_er.push_back(read.enc2_er);
                                z1[k].vrms1.push_back(read.rms1);
                                z1[k].vrms2.push_back(read.rms2);
                                z1[k].vrms3.push_back(read.rms3);
                                z1[k].vrms4.push_back(read.rms4);
                                z1[k].vrms1_er.push_back(read.rms1_er);
                                z1[k].vrms3_er.push_back(read.rms3_er);
                                z1[k].vpa.push_back(read.pa);
                                z1[k].vpa_er.push_back(read.pa_er);
                                z1[k].vtemp.push_back(read.temp);
                                z1[k].vp_dac.push_back(read.p_dac);
                                z1[k].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is gain and each entry is identified by the read gain-id {0,1,2,3,4,5,6,7}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" << read.peaktime << 
                                " rms_acqoff:" << z1[k].vrms1[read.gain] << " rms acqoff error: " << z1[k].vrms1_er[read.gain] << endl;
                            } //check value of read channel
                        } //loop through 5 chosen channels
                    }else{ // if capacitance is on x axis, the 2nd constant is gain, so we only want data with a specific gain
                        if(read.gain==slct1.arg_int[6]){ //if the current read gain is equal to the value chosen for constant2
                            for(int k=0;k<6;k++){
                                if(read.chan==slct1.chn_id[k]){ //when channel is on z axis, only 5 chosen channels are plotted,
                                                                //here, we loop through all chosen channels, and check whether the
                                                                //current read channel is one of them
                                    //CHOICE:: Y: ANY, X: CAPACITANCE, Z: CHANNEL, CONSTANT1: PEAKTIME, CONSTANT2: GAIN
                                    z1[k].chan = read.chan; //each z axis obj corresponds to a channel of certain value{0-63} 
                                                            //this value is saved to be used later in the legend
                                    //save current chosen read values to vectors inside z1 vector obj.
                                    z1[k].venc1.push_back(read.enc1);
                                    z1[k].venc2.push_back(read.enc2);
                                    z1[k].venc1_er.push_back(read.enc1_er);
                                    z1[k].venc2_er.push_back(read.enc2_er);
                                    z1[k].vrms1.push_back(read.rms1);
                                    z1[k].vrms2.push_back(read.rms2);
                                    z1[k].vrms3.push_back(read.rms3);
                                    z1[k].vrms4.push_back(read.rms4);
                                    z1[k].vrms1_er.push_back(read.rms1_er);
                                    z1[k].vrms3_er.push_back(read.rms3_er);
                                    z1[k].vpa.push_back(read.pa);
                                    z1[k].vpa_er.push_back(read.pa_er);
                                    z1[k].vtemp.push_back(read.temp);
                                    z1[k].vp_dac.push_back(read.p_dac);
                                    z1[k].vp_dac_er.push_back(read.p_dac_er);
                                    //print the chosen data line
                                    //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                    //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7,8}.
                                    cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                    << read.peaktime << " cap:" << read.cap << " rms_acqoff:" << z1[k].vrms1[read.cap] << 
                                    " rms acqoff error: " << z1[k].vrms1_er[read.cap] << endl;   
                                } //check value of read channel
                            } //loop through 5 chosen channels
                        } //check value of constant2 (gain)
                    }  //check if capacitance is on x axis
                    break;
                case 2: // z axis(or 2nd const.): gain
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have channel or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if channel is on x axis:
                        //CHOICE:: Y: ANY, X: CHANNEL, Z(OR 2ND CONST.): GAIN, CONSTANT1: PEAKTIME
                        z1[read.gain].gain = read.gain; //each z axis obj corresponds to a gain of certain value{0,1,2,3,4,5,6,7} 
                                                        //this value is saved to be used later in the legend
                        //save current chosen read values to vectors inside z1 vector obj.
                        z1[read.gain].venc1.push_back(read.enc1);
                        z1[read.gain].venc2.push_back(read.enc2);
                        z1[read.gain].venc1_er.push_back(read.enc1_er);
                        z1[read.gain].venc2_er.push_back(read.enc2_er);
                        z1[read.gain].vrms1.push_back(read.rms1);
                        z1[read.gain].vrms2.push_back(read.rms2);
                        z1[read.gain].vrms3.push_back(read.rms3);
                        z1[read.gain].vrms4.push_back(read.rms4);
                        z1[read.gain].vrms1_er.push_back(read.rms1_er);
                        z1[read.gain].vrms3_er.push_back(read.rms3_er);
                        z1[read.gain].vpa.push_back(read.pa);
                        z1[read.gain].vpa_er.push_back(read.pa_er);
                        z1[read.gain].vtemp.push_back(read.temp);
                        z1[read.gain].vp_dac.push_back(read.p_dac);
                        z1[read.gain].vp_dac_er.push_back(read.p_dac_er);
                        //print the chosen data line
                        //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                        //here x axis is channel and each entry is identified by the read channel-id {0-63}.
                        cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                        << read.peaktime << " enc_acqon:" << z1[read.gain].venc2[read.chan] << " enc acqon error: " 
                        << z1[read.gain].venc2_er[read.chan] << endl;
                    }else{ // if capacitance is on x axis, the 2nd constant is channel, so we only want data with a specific channel
                        if(read.chan==slct1.arg_int[6]){ //if the current read channel is equal to the value chosen for constant2
                            //CHOICE:: Y: ANY, X: CAPACITANCE, Z: GAIN, CONSTANT1: PEAKTIME, CONSTANT2: CHANNEL
                            z1[read.gain].gain = read.gain; //each z axis obj corresponds to a gain of certain value{0,1,2,3,4,5,6,7} 
                                                            //this value is saved to be used later in the legend
                            //save current chosen read values to vectors inside z1 vector obj.
                            z1[read.gain].venc1.push_back(read.enc1);
                            z1[read.gain].venc2.push_back(read.enc2);
                            z1[read.gain].venc1_er.push_back(read.enc1_er);
                            z1[read.gain].venc2_er.push_back(read.enc2_er);
                            z1[read.gain].vrms1.push_back(read.rms1);
                            z1[read.gain].vrms2.push_back(read.rms2);
                            z1[read.gain].vrms3.push_back(read.rms3);
                            z1[read.gain].vrms4.push_back(read.rms4);
                            z1[read.gain].vrms1_er.push_back(read.rms1_er);
                            z1[read.gain].vrms3_er.push_back(read.rms3_er);
                            z1[read.gain].vpa.push_back(read.pa);
                            z1[read.gain].vpa_er.push_back(read.pa_er); 
                            z1[read.gain].vtemp.push_back(read.temp); 
                            z1[read.gain].vp_dac.push_back(read.p_dac);
                            z1[read.gain].vp_dac_er.push_back(read.p_dac_er);
                            //print the chosen data line
                            //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                            //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7,8}.
                            cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" << 
                            read.peaktime << " cap:" << read.cap << " rms_acqoff:" << z1[read.gain].vrms1[read.cap] << 
                            " rms acqoff error: " << z1[read.gain].vrms1_er[read.cap] << endl;
                        } // check value of constant2 (channel)
                    } //check if capacitance is on x axis
                    break;
                case 3: //z axis: capacitance: in this case x axis is definitely  
                        //not capacitance, but also again a 2nd constant exists
                    switch(slct1.arg_int[5]){ //check constant2 type
                        case 0: //constant2: channel
                            if(read.chan==slct1.arg_int[6]){ //if the current read channel is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: GAIN, Z: CAPACITANCE, CONSTANT1: PEAKTIME, CONSTANT2: CHANNEL
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);
                                z1[read.cap].vtemp.push_back(read.temp);
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is gain and each entry is identified by the read gain-id {0,1,2,3,4,5,6,7}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" rms_acqoff:" << z1[read.cap].vrms1[read.gain] <<
                                " rms acqoff error: " << z1[read.cap].vrms1_er[read.gain] << endl;
                            }
                            break;
                        case 2: //constant 2: gain
                            if(read.gain==slct1.arg_int[6]){ //if the current read gain is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: CHANNEL, Z: CAPACITANCE, CONSTANT1: PEAKTIME, CONSTANT2: GAIN
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);
                                z1[read.cap].vtemp.push_back(read.temp);
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is channel and each entry is identified by the read channel-id {0-63}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" enc_acqon:" << z1[read.cap].venc2[read.chan] <<
                                " enc acqon error: " << z1[read.cap].venc2_er[read.chan] << endl;                                           
                            } //check value of constant2 (gain)
                            break;
                    } // switch constant2
                    break;
            } //switch z axis
        } //check value of constant1 (peaktime)
        break;
    case 2:  //constant1 = "gain"
        if(read.gain==slct1.arg_int[4]){ //if the current read gain is equal to the value chosen for constant1
            switch(slct1.arg_int[3]){ //select z axis (in case no z axis is chosen and z!=capacitance, this argument takes the 2nd constant type
                case 0: // z axis(or 2nd const.): channel
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have peaktime or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if peaktime is on x axis:                        
                        for(int k=0;k<6;k++){ 
                            if(read.chan==slct1.chn_id[k]){ //when channel is on z axis, only 5 chosen channels are plotted,
                                                            //here, we loop through all chosen channels, and check whether the
                                                            //current read channel is one of them
                                //CHOICE:: Y: ANY, X: PEAKTIME, Z(OR 2ND CONST.): CHANNEL, CONSTANT1: GAIN
                                z1[k].chan = read.chan; //each z axis obj corresponds to a channel of certain value{0-63} 
                                                        //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[k].venc1.push_back(read.enc1);
                                z1[k].venc2.push_back(read.enc2);
                                z1[k].venc1_er.push_back(read.enc1_er);
                                z1[k].venc2_er.push_back(read.enc2_er);
                                z1[k].vrms1.push_back(read.rms1);
                                z1[k].vrms2.push_back(read.rms2);
                                z1[k].vrms3.push_back(read.rms3);
                                z1[k].vrms4.push_back(read.rms4);
                                z1[k].vrms1_er.push_back(read.rms1_er);
                                z1[k].vrms3_er.push_back(read.rms3_er);
                                z1[k].vpa.push_back(read.pa);
                                z1[k].vpa_er.push_back(read.pa_er);
                                z1[k].vtemp.push_back(read.temp);
                                z1[k].vp_dac.push_back(read.p_dac);
                                z1[k].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is peaktime and each entry is identified by the read peaktime-id {0,1,2,3}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " rms_acqoff:" << z1[k].vrms1[read.peaktime] << " rms acqoff error: " 
                                << z1[k].vrms1_er[read.peaktime] << endl;
                            } //check value of read channel
                        } //loop through 5 chosen channels
                    }else{ // if capacitance is on x axis, the 2nd constant is peaktime, so we only want data with a specific peaktime
                        if(read.peaktime==slct1.arg_int[6]){ //if the current read gain is equal to the value chosen for constant2
                            for(int k=0;k<6;k++){ 
                                if(read.chan==slct1.chn_id[k]){ //when channel is on z axis, only 5 chosen channels are plotted,
                                                                //here, we loop through all chosen channels, and check whether the
                                                                //current read channel is one of them
                                    //CHOICE:: Y: ANY, X: CAPACITANCE, Z: CHANNEL, CONSTANT1: GAIN, CONSTANT2: PEAKTIME
                                    z1[k].chan = read.chan; //each z axis obj corresponds to a channel of certain value{0-63} 
                                                            //this value is saved to be used later in the legend
                                    //save current chosen read values to vectors inside z1 vector obj.
                                    z1[k].venc1.push_back(read.enc1);
                                    z1[k].venc2.push_back(read.enc2);
                                    z1[k].venc1_er.push_back(read.enc1_er);
                                    z1[k].venc2_er.push_back(read.enc2_er);
                                    z1[k].vrms1.push_back(read.rms1);
                                    z1[k].vrms2.push_back(read.rms2);
                                    z1[k].vrms3.push_back(read.rms3);
                                    z1[k].vrms4.push_back(read.rms4);
                                    z1[k].vrms1_er.push_back(read.rms1_er);
                                    z1[k].vrms3_er.push_back(read.rms3_er);
                                    z1[k].vpa.push_back(read.pa);
                                    z1[k].vpa_er.push_back(read.pa_er);
                                    z1[k].vtemp.push_back(read.temp);
                                    z1[k].vp_dac.push_back(read.p_dac);
                                    z1[k].vp_dac_er.push_back(read.p_dac_er);
                                    //print the chosen data line
                                    //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                    //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7}.
                                    cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << 
                                    " pt:" << read.peaktime << " rms_acqoff:" << z1[k].vrms1[read.cap] << " rms acqoff error: " 
                                    << z1[k].vrms1_er[read.cap] << endl;    
                                } //check value of read channel
                            } //loop through 5 chosen channels                                
                        } //check value of constant2 (peaktime)
                    }// check if capacitance is on x axis
                    break;
                case 1: //z axis(or 2nd const.): peaktime
                    if(slct1.arg_int[2]!=3){ //here there are two options: either we have channel or capacitance on x axis
                                             // if we have capacitance on x axis, then it means that a 2nd constant exists
                                             // and hence we must also sift our data through constant2 as well
                                             // so if channel is on x axis:
                        //CHOICE:: Y: ANY, X: CHANNEL, Z(OR 2ND CONST.): PEAKTIME, CONSTANT1: GAIN
                        z1[read.peaktime].peaktime = read.peaktime; //each z axis obj corresponds to a peaktime of certain value{0,1,2,3} 
                                                                    //this value is saved to be used later in the legend
                        //save current chosen read values to vectors inside z1 vector obj.
                        z1[read.peaktime].venc1.push_back(read.enc1);
                        z1[read.peaktime].venc2.push_back(read.enc2);
                        z1[read.peaktime].venc1_er.push_back(read.enc1_er);
                        z1[read.peaktime].venc2_er.push_back(read.enc2_er);
                        z1[read.peaktime].vrms1.push_back(read.rms1);
                        z1[read.peaktime].vrms2.push_back(read.rms2);
                        z1[read.peaktime].vrms3.push_back(read.rms3);
                        z1[read.peaktime].vrms4.push_back(read.rms4);
                        z1[read.peaktime].vrms1_er.push_back(read.rms1_er);
                        z1[read.peaktime].vrms3_er.push_back(read.rms3_er);
                        z1[read.peaktime].vpa.push_back(read.pa);
                        z1[read.peaktime].vpa_er.push_back(read.pa_er);
                        z1[read.peaktime].vtemp.push_back(read.temp);
                        z1[read.peaktime].vp_dac.push_back(read.p_dac);
                        z1[read.peaktime].vp_dac_er.push_back(read.p_dac_er);
                        //print the chosen data line
                        //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                        //here x axis is channel and each entry is identified by the read channel-id {0-63}.
                        cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                        << read.peaktime << " rms_acqoff:" << z1[read.peaktime].vrms1[read.chan] <<
                        " rms acqoff error: " << z1[read.peaktime].vrms1_er[read.chan] << endl;
                    }else{ // if capacitance is on x axis, the 2nd constant is channel, so we only want data with a specific channel
                        if(read.chan==slct1.arg_int[6]){ //if the current read channel is equal to the value chosen for constant2
                            //CHOICE:: Y: ANY, X: CAPACITANCE, Z: PEAKTIME, CONSTANT1: GAIN, CONSTANT2: CHANNEL
                            z1[read.peaktime].peaktime = read.peaktime; //each z axis obj corresponds to a peaktime of certain value{0,1,2,3} 
                                                                        //this value is saved to be used later in the legend
                            //save current chosen read values to vectors inside z1 vector obj.
                            z1[read.peaktime].venc1.push_back(read.enc1);
                            z1[read.peaktime].venc2.push_back(read.enc2);
                            z1[read.peaktime].venc1_er.push_back(read.enc1_er);
                            z1[read.peaktime].venc2_er.push_back(read.enc2_er);
                            z1[read.peaktime].vrms1.push_back(read.rms1);
                            z1[read.peaktime].vrms2.push_back(read.rms2);
                            z1[read.peaktime].vrms3.push_back(read.rms3);
                            z1[read.peaktime].vrms4.push_back(read.rms4);
                            z1[read.peaktime].vrms1_er.push_back(read.rms1_er);
                            z1[read.peaktime].vrms3_er.push_back(read.rms3_er);
                            z1[read.peaktime].vpa.push_back(read.pa);
                            z1[read.peaktime].vpa_er.push_back(read.pa_er);
                            z1[read.peaktime].vtemp.push_back(read.temp);
                            z1[read.peaktime].vp_dac.push_back(read.p_dac);
                            z1[read.peaktime].vp_dac_er.push_back(read.p_dac_er);
                            //print the chosen data line
                            //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                            //here x axis is capacitance and each entry is identified by the read cap-id {0,1,2,3,4,5,6,7,8}.
                            cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                            << read.peaktime << " rms_acqoff:" << z1[read.peaktime].vrms1[read.cap] << " rms acqoff error: " 
                            << z1[read.peaktime].vrms1_er[read.cap] << endl;
                        } //check value of contant2 (channel)
                    } //check if capacitance is on x axis
                    break;
                case 3: //z axis: capacitance: in this case x axis is definitely  
                        //not capacitance, but also again a 2nd constant exists
                    switch(slct1.arg_int[5]){ //select constant2 type
                        case 0: //constant2: channel
                            if(read.chan==slct1.arg_int[6]){ //if the current read channel is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: PEAKTIME, Z: CAPACITANCE, CONSTANT1: GAIN, CONSTANT2: CHANNEL
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);   
                                z1[read.cap].vtemp.push_back(read.temp);  
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);                                       
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is peaktime and each entry is identified by the read peaktime-id {0,1,2,3}.
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" rms_acqoff:" << z1[read.cap].vrms1[read.peaktime] 
                                << " rms acqoff error: " << z1[read.cap].vrms1_er[read.peaktime] << endl;
                            }
                            break;
                        case 1: //constant2: peaktime
                            if(read.peaktime==slct1.arg_int[6]){ //if the current read peaktime is equal to the value chosen for constant2
                                //CHOICE:: Y: ANY, X: CHANNEL, Z: CAPACITANCE, CONSTANT1: GAIN, CONSTANT2: PEAKTIME
                                z1[read.cap].cap = read.cap; //each z axis obj corresponds to a capacitance of certain value{0,1,2,3,4,5,6,7,8} 
                                                             //this value is saved to be used later in the legend
                                //save current chosen read values to vectors inside z1 vector obj.
                                z1[read.cap].venc1.push_back(read.enc1);
                                z1[read.cap].venc2.push_back(read.enc2);
                                z1[read.cap].venc1_er.push_back(read.enc1_er);
                                z1[read.cap].venc2_er.push_back(read.enc2_er);
                                z1[read.cap].vrms1.push_back(read.rms1);
                                z1[read.cap].vrms2.push_back(read.rms2);
                                z1[read.cap].vrms3.push_back(read.rms3);
                                z1[read.cap].vrms4.push_back(read.rms4);
                                z1[read.cap].vrms1_er.push_back(read.rms1_er);
                                z1[read.cap].vrms3_er.push_back(read.rms3_er);
                                z1[read.cap].vpa.push_back(read.pa);
                                z1[read.cap].vpa_er.push_back(read.pa_er);
                                z1[read.cap].vtemp.push_back(read.temp);
                                z1[read.cap].vp_dac.push_back(read.p_dac);
                                z1[read.cap].vp_dac_er.push_back(read.p_dac_er);
                                //print the chosen data line
                                //the enc/rms/pulse vectors of z1 obj will have as many entries as there are values on x axis
                                //here x axis is channel and each entry is identified by the read channel-id {0-63}.                        
                                cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" 
                                << read.peaktime << " cap:" << read.cap <<" rms_acqoff:" << z1[read.cap].vrms1[read.chan] <<
                                " rms acqoff error: " << z1[read.cap].vrms1_er[read.chan] << endl;
                            } //check value of contant2 (peaktime)
                            break;
                    } //switch constant2 
                    break;
            } //switch z axis
        } //check value of constant1 (gain)
        break;
} //switch constant1

return z1;
} //end of fillvectors

void FillGraphs(vector<TGraphErrors*> &g0,selection slct1,z read,vector<z> z1){ //Fill Graphs. This function takes as arguments,
                                                                                //1: a vector of TGraphError objs. 
                                                                                //2: the user selection
                                                                                //3: a z type read obj that holds temporary read values
                                                                                //the read obj is necessary because it holds the last read 
                                                                                //values in the data, and hence it is useful in order to 
                                                                                //determine the actual length of the x axis
                                                                                //4: a vector of z objs. which hold the chosen data
//FILL GRAPHS

    //x axis vectors
    vector<double> gain_id = {0.5,1.,3.,4.5,6.,9.,12.,16.};
    vector<double> gain_id_er = {0.,0.,0.,0.,0.,0.,0.,0.};
    int gnsize = gain_id.size();
    vector<double> pt_id = {200.,100.,50.,25.};
    vector<double> pt_id_er = {0.,0.,0.,0.};
    int ptsize = pt_id.size();
    vector<double> chan_id;
    vector<double> chan_id_er;
    for(double i=0.;i<=read.chan;i+=1.){
        chan_id.push_back(i);
        chan_id_er.push_back(0.);
    }
    int chansize = chan_id.size();
    vector<double> cap_val = {0.,8.,30.,76.,98.,338.,360.,406.,428.}; //to use as x axis
    //vector<double> cap_val = {0.,8.,38.,76.,106.};
    vector<double> cap_val_er = {0.,0.,0.,0.,0.,0.,0.,0.,0.};
    //vector<double> cap_val_er = {0.,0.,0.,0.,0.};
    int capsize = cap_val.size();

    int tempsize = z1[0].vtemp.size();
    vector<double> temp_er;
    for(int i=0;i<tempsize;i++){
        temp_er.push_back(0.);
    }

    switch(slct1.arg_int[1]){
        case 0: //case y is rms acq on
            switch(slct1.arg_int[2]){
                case 0: //case x is channel
                    if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                        for (int i = 0; i < z1.size(); i++) {
                            g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].vrms4[0], &chan_id_er[0], &z1[i].vrms3_er[0]);
                    }
                }
                else {
                    g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vrms4[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
                }
                break;
            case 1: //case x is peaktime
                if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                    for (int i = 0; i < z1.size(); i++) {
                        g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].vrms4[0], &pt_id_er[0], &z1[i].vrms3_er[0]);
                    }
                }
                else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                    g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].vrms4[0], &pt_id_er[0], &z1[0].vrms3_er[0]);
                }
                else {
                    g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vrms4[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
                }
                break;
            case 2: //case x is gain
                if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                    for (int i = 0; i < z1.size(); i++) {
                        g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].vrms4[0], &gain_id_er[0], &z1[i].vrms3_er[0]);
                    }
                }
                else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                    g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].vrms4[0], &gain_id_er[0], &z1[0].vrms3_er[0]);
                }
                else {
                    g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vrms4[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
                }
                break;
            case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].vrms4[0], &cap_val_er[0], &z1[i].vrms3_er[0]);
                } 
                break; 
        }
        break;        
    case 1: //case y is rms acq off
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].vrms2[0], &chan_id_er[0], &z1[i].vrms1_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vrms2[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].vrms2[0], &pt_id_er[0], &z1[0].vrms1_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].vrms2[0], &pt_id_er[0], &z1[0].vrms1_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vrms2[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].vrms2[0], &gain_id_er[0], &z1[i].vrms1_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].vrms2[0], &gain_id_er[0], &z1[0].vrms1_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vrms2[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].vrms2[0], &cap_val_er[0], &z1[i].vrms1_er[0]);
                } 
                break;
        }
        break;
    case 2: // case y is enc acq on
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].venc2[0], &chan_id_er[0], &z1[i].venc2_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].venc2[0], &chan_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].venc2[0], &pt_id_er[0], &z1[i].venc2_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].venc2[0], &pt_id_er[0], &z1[0].venc2_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].venc2[0], &pt_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].venc2[0], &gain_id_er[0], &z1[i].venc2_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].venc2[0], &gain_id_er[0], &z1[0].venc2_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].venc2[0], &gain_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].venc2[0], &cap_val_er[0], &z1[i].venc2_er[0]);
                } 
                break;
        }
        break;
    case 3: //case y is enc acq off
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].venc1[0], &chan_id_er[0], &z1[i].venc1_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].venc1[0], &chan_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].venc1[0], &pt_id_er[0], &z1[i].venc1_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].venc1[0], &pt_id_er[0], &z1[0].venc1_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].venc1[0], &pt_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].venc1[0], &gain_id_er[0], &z1[i].venc1_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].venc1[0], &gain_id_er[0], &z1[0].venc1_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].venc1[0], &gain_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].venc1[0], &cap_val_er[0], &z1[i].venc1_er[0]);
                } 
                break;
        }
        break;
    case 4: //case y is enc
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].venc1[0], &chan_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            g0[1] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].venc2[0], &chan_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            break;
        case 1: //case x is peaktime            
            g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].venc1[0], &pt_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            g0[1] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].venc2[0], &pt_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            break;
        case 2: //case x is gain
            g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].venc1[0], &gain_id_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            g0[1] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].venc2[0], &gain_id_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            break;
        case 3: //case x is capacitance (then z won't be capacitance)
            g0[0] = new TGraphErrors(capsize, &cap_val[0], &z1[slct1.arg_int[6]].venc1[0], &cap_val_er[0], &z1[slct1.arg_int[6]].venc1_er[0]);
            g0[1] = new TGraphErrors(capsize, &cap_val[0], &z1[slct1.arg_int[6]].venc2[0], &cap_val_er[0], &z1[slct1.arg_int[6]].venc2_er[0]);
            break;
        }
    break;
    case 5: //case y is rms
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vrms2[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            g0[1] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vrms4[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
            break;
        case 1: //case x is peaktime            
            g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vrms2[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            g0[1] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vrms4[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
            break;
        case 2: //case x is gain
            g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vrms2[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            g0[1] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vrms4[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
            break;
        case 3: //case x is capacitance (then z won't be capacitance)
            g0[0] = new TGraphErrors(capsize, &cap_val[0], &z1[slct1.arg_int[6]].vrms2[0], &cap_val_er[0], &z1[slct1.arg_int[6]].vrms1_er[0]);
            g0[1] = new TGraphErrors(capsize, &cap_val[0], &z1[slct1.arg_int[6]].vrms4[0], &cap_val_er[0], &z1[slct1.arg_int[6]].vrms3_er[0]);
            break;
        }
    break;
    case 6: //case y is pulse ampl
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].vpa[0], &chan_id_er[0], &z1[i].vpa_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vpa[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vpa_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].vpa[0], &pt_id_er[0], &z1[i].vpa_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].vpa[0], &pt_id_er[0], &z1[0].vpa_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vpa[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vpa_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].vpa[0], &gain_id_er[0], &z1[i].vpa_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].vpa[0], &gain_id_er[0], &z1[0].vpa_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vpa[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vpa_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].vpa[0], &cap_val_er[0], &z1[i].vpa_er[0]);
                } 
                break;
        case 4: //case x is temperature
                
              //  for (int i = 0; i < z1.size(); i++) {
                g0[0] = new TGraphErrors(tempsize, &z1[slct1.arg_int[6]].vtemp[0],&z1[slct1.arg_int[6]].vpa[0],&temp_er[0],&z1[slct1.arg_int[6]].vpa_er[0]);
               // }
                break;
        }
    break;
    case 7: //case y is temperature
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].vtemp[0], &chan_id_er[0], &temp_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vtemp[0], &chan_id_er[0], &temp_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].vtemp[0], &pt_id_er[0], &temp_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].vtemp[0], &pt_id_er[0], &temp_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vtemp[0], &pt_id_er[0], &temp_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].vtemp[0], &gain_id_er[0], &temp_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].vtemp[0], &gain_id_er[0], &temp_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vtemp[0], &gain_id_er[0], &temp_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].vtemp[0], &cap_val_er[0], &temp_er[0]);
                } 
                break;
        }
    break;
    case 8: //case y is pulser step
        switch (slct1.arg_int[2]) {
        case 0: //case x is channel
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(chansize, &chan_id[0], &z1[i].vp_dac[0], &chan_id_er[0], &z1[i].vp_dac_er[0]);
                }
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(chansize, &chan_id[0], &z1[slct1.arg_int[6]].vp_dac[0], &chan_id_er[0], &z1[slct1.arg_int[6]].vp_dac_er[0]);
            }
            break;
        case 1: //case x is peaktime
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(ptsize, &pt_id[0], &z1[i].vp_dac[0], &pt_id_er[0], &z1[i].vp_dac_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[0].vp_dac[0], &pt_id_er[0], &z1[0].vp_dac_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(ptsize, &pt_id[0], &z1[slct1.arg_int[6]].vp_dac[0], &pt_id_er[0], &z1[slct1.arg_int[6]].vp_dac_er[0]);
            }
            break;
        case 2: //case x is gain
            if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3) {
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(gnsize, &gain_id[0], &z1[i].vp_dac[0], &gain_id_er[0], &z1[i].vp_dac_er[0]);
                }
            }
            else if (slct1.arg_int[5] == 0 && slct1.arg_int[3]!=3) {
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[0].vp_dac[0], &gain_id_er[0], &z1[0].vp_dac_er[0]);
            }
            else if(slct1.arg_int[5] > 0 && slct1.arg_int[3]!=3){
                g0[0] = new TGraphErrors(gnsize, &gain_id[0], &z1[slct1.arg_int[6]].vp_dac[0], &gain_id_er[0], &z1[slct1.arg_int[6]].vp_dac_er[0]);
            }
            break;
        case 3: //case x is capacitance
                for (int i = 0; i < z1.size(); i++) {
                    g0[i] = new TGraphErrors(capsize, &cap_val[0], &z1[i].vp_dac[0], &cap_val_er[0], &z1[i].vp_dac_er[0]);
                } 
                break;
        case 4: //case x is temperature
                g0[0] = new TGraphErrors(tempsize, &z1[slct1.arg_int[6]].vtemp[0],&z1[slct1.arg_int[6]].vp_dac[0],&temp_er[0],&z1[slct1.arg_int[6]].vp_dac_er[0]);
                break;
        }
    break;
}

}

void CustomizeGraphs(vector<TGraphErrors*> &g0){
//customize graphs
    for(int i=0;i<g0.size();i++){
        g0[i]->SetMarkerSize(1.5);
    }

    switch(g0.size()){
        case 1:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
            break;
        case 2:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
    
            g0[1]->SetMarkerStyle(20);
            g0[1]->SetMarkerColor(kRed);
            break;
        case 4:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
    
            g0[1]->SetMarkerStyle(20);
            g0[1]->SetMarkerColor(kRed);

            g0[2]->SetMarkerStyle(20);
            g0[2]->SetMarkerColor(kBlack);
    
            g0[3]->SetMarkerStyle(21);
            g0[3]->SetMarkerColor(kGreen);
            break;
        case 6:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
    
            g0[1]->SetMarkerStyle(20);
            g0[1]->SetMarkerColor(kRed);

            g0[2]->SetMarkerStyle(20);
            g0[2]->SetMarkerColor(kBlack);
    
            g0[3]->SetMarkerStyle(21);
            g0[3]->SetMarkerColor(kGreen);

            g0[4]->SetMarkerStyle(21);
            g0[4]->SetMarkerColor(kOrange);

            g0[5]->SetMarkerStyle(21);
            g0[5]->SetMarkerColor(kRed);
            break;
        case 8:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
    
            g0[1]->SetMarkerStyle(20);
            g0[1]->SetMarkerColor(kViolet);

            g0[2]->SetMarkerStyle(20);
            g0[2]->SetMarkerColor(kBlack);
    
            g0[3]->SetMarkerStyle(21);
            g0[3]->SetMarkerColor(kGreen);

            g0[4]->SetMarkerStyle(21);
            g0[4]->SetMarkerColor(kOrange);

            g0[5]->SetMarkerStyle(21);
            g0[5]->SetMarkerColor(kRed);

            g0[6]->SetMarkerStyle(20);
            g0[6]->SetMarkerColor(kGreen);

            g0[7]->SetMarkerStyle(20);
            g0[7]->SetMarkerColor(kRed);
            break;
        case 9:
            g0[0]->SetMarkerStyle(20);
            g0[0]->SetMarkerColor(kBlue);
    
            g0[1]->SetMarkerStyle(20);
            g0[1]->SetMarkerColor(kViolet);

            g0[2]->SetMarkerStyle(20);
            g0[2]->SetMarkerColor(kBlack);
    
            g0[3]->SetMarkerStyle(21);
            g0[3]->SetMarkerColor(kGreen);

            g0[4]->SetMarkerStyle(21);
            g0[4]->SetMarkerColor(kOrange);

            g0[5]->SetMarkerStyle(21);
            g0[5]->SetMarkerColor(kRed);

            g0[6]->SetMarkerStyle(20);
            g0[6]->SetMarkerColor(kGreen);

            g0[7]->SetMarkerStyle(20);
            g0[7]->SetMarkerColor(kRed);

            g0[8]->SetMarkerStyle(20);
            g0[8]->SetMarkerColor(kGray);
            break;
    }
}

void CreateLegend(vector<TGraphErrors*> &g0, selection slct1,vector<z> z1){
//create legend names
vector<string> legnames;
string legtitle;
if (slct1.arg_int[5] < 0 || slct1.arg_int[3]==3 || slct1.arg_int[2]==3) {
    switch (slct1.arg_int[3]) {
    case 0:
        legtitle = "CHANNEL";
        for (int i = 0; i < 6; i++) {
            string chnid = std::to_string(slct1.chn_id[i]);
            legnames.push_back(chnid);
        }
        break;
    case 1:
        legtitle = "PEAKTIME";
        for (int i = 0; i < 4; i++) {
            switch (z1[i].peaktime) {
            case 0:
                legnames.push_back("200 ns");
                break;
            case 1:
                legnames.push_back("100 ns");
                break;
            case 2:
                legnames.push_back("50 ns");
                break;
            case 3:
                legnames.push_back("25 ns");
                break;
            }
        }
        break;
    case 2:
        legtitle = "GAIN";
        for (int i = 0; i < 8; i++) {
            switch (z1[i].gain) {
            case 0:
                legnames.push_back("0.5 mV/fC");
                break;
            case 1:
                legnames.push_back("1 mV/fC");
                break;
            case 2:
                legnames.push_back("3 mV/fC");
                break;
            case 3:
                legnames.push_back("4.5 mV/fC");
                break;
            case 4:
                legnames.push_back("6 mV/fC");
                break;
            case 5:
                legnames.push_back("9 mV/fC");
                break;
            case 6:
                legnames.push_back("12 mV/fC");
                break;
            case 7:
                legnames.push_back("16 mV/fC");
                break;
            }
        }
        break;
    case 3:
        legtitle="CAPACITANCE";
        for (int i = 0; i < 9; i++) {
         switch (z1[i].cap) {
            case 0:
                legnames.push_back("0 pF");
                break;
            case 1:
                if(z1.size()==2){
                    legnames.push_back("30 pF");
                }else{
                    legnames.push_back("8 pF");
                }
                break;
            case 2:
                legnames.push_back("30 pF");
                break;
            case 3:
                legnames.push_back("76 pF");
                break;
            case 4:
                legnames.push_back("98 pF");
                break;
            case 5:
                legnames.push_back("338 pF");
                break;
            case 6:
                legnames.push_back("360 pF");
                break;
            case 7:
                legnames.push_back("406 pF");
                break;
            case 8: 
                legnames.push_back("428 pF");
                break;   
         }
        }
        break;
    }
    //create legend
    TLegend* leg = new TLegend(0.91, 0.78, 0.99, 0.98);
    leg->SetHeader(legtitle.c_str());
    leg->SetBorderSize(1);
    leg->SetFillStyle(0);
    for (int i = 0; i < g0.size(); i++) {
        leg->AddEntry(g0[i], legnames[i].c_str(), "P");
    }
    leg->Draw("SAME");
}else if (slct1.arg_int[5] >= 0 && (slct1.arg_int[1]==4 || slct1.arg_int[1]==5)) {
    switch (slct1.arg_int[1]) {
    case 4: 
        legnames.push_back("enc acq off");
        legnames.push_back("enc acq on");
        break;
    case 5:
        legnames.push_back("rms acq off");
        legnames.push_back("rms acq on");
        break;
    }
    //create legend
    TLegend* leg = new TLegend(0.15, 0.68, 0.45, 0.88);
    //leg->SetHeader(legtitle.c_str());
    leg->SetBorderSize(1);
    leg->SetFillStyle(0);
    for (int i = 0; i < 2; i++) {
        leg->AddEntry(g0[i], legnames[i].c_str(), "P");
    }
    leg->Draw("SAME");
}
else {
    cout << "no legend" << endl;
}

}
void PlotVectors(selection slct1, z read, vector<z> z1){ //Plot the filled vectors. This function takes as arguments:
                                                         //1: the user selection, 2: a type z read obj which holds temporary read values,
                                                         //the read obj is necessary because it holds the last read values in the data,
                                                         //and hence it is useful in order to determine the actual length of the x axis
                                                         //3: a type z vector obj to separate between graphs on the z axis 
                                                         //and fill corresponding values. 
    //INITIALIZE GRAPHS
    vector<TGraphErrors*> g0; //vector of error-graphs to plot
    //determine size of g0
    if(slct1.arg_int[5]<0 || slct1.arg_int[3]==3 || slct1.arg_int[2]==3){ //
        for(int places=0;places<z1.size();places++){
            g0.push_back(new TGraphErrors());
        }
    }else if(slct1.arg_int[5]>=0 && (slct1.arg_int[1]==4 || slct1.arg_int[1]==5)){
        g0.push_back(new TGraphErrors());
        g0.push_back(new TGraphErrors());
    }else{
        g0.push_back(new TGraphErrors());
    }

    

    TMultiGraph *mg = new TMultiGraph();
    mg->SetTitle(slct1.title.c_str());
    if(slct1.arg_int[1]==0 || slct1.arg_int[1]==1 || slct1.arg_int[1]==5){
        mg->GetYaxis()->SetTitle("RMS (mV)");
    } else if(slct1.arg_int[1]==2 || slct1.arg_int[1]==3 || slct1.arg_int[1]==4){
        mg->GetYaxis()->SetTitle("ENC/e");
    } else if(slct1.arg_int[1]==6){
        mg->GetYaxis()->SetTitle("Pulse Amplitude (mV)");
    } else if(slct1.arg_int[1]==7){
        mg->GetYaxis()->SetTitle("Temperature (grad celcius)");
    } else if(slct1.arg_int[1]==8){
        mg->GetYaxis()->SetTitle("Pulser Step (mV)");
    }

    switch(slct1.arg_int[2]){
        case 0:
            mg->GetXaxis()->SetTitle("Channel");
            break;
        case 1:
            mg->GetXaxis()->SetTitle("Peaktime (ns)");
            break;
        case 2:
            mg->GetXaxis()->SetTitle("Gain (mV/fC)");
            break;
        case 3:
            mg->GetXaxis()->SetTitle("External Capacitance (pF)");
            break;
    }

FillGraphs(g0,slct1,read,z1);
CustomizeGraphs(g0);
TCanvas *c1 = new TCanvas("c1", "c1",67,55,1853,1025);
for(int i=0;i<g0.size();i++){
mg->Add(g0[i]);
}
mg->Draw("AP");
CreateLegend(g0,slct1,z1);
ostringstream imagenamestream = CreateFilename(slct1);
imagenamestream << ".png";
string imagename = imagenamestream.str();
ostringstream filenamestream = CreateFilename(slct1);
filenamestream << ".C";
string filename = filenamestream.str();
c1->SaveAs(imagename.c_str());
c1->SaveAs(filename.c_str());
}




int ReadData(selection slct1) {

    //Declare arrays and variables
   

    if(slct1.arg[0]!="peaktime" && slct1.arg[0]!="gain" && slct1.arg[0]!="channel"){
        cout << "not a valid selection" << endl;
        return 1;
    } 

    

    if (slct1.arg[5] != "peaktime" && slct1.arg[5] != "gain" && slct1.arg[5] != "channel" && slct1.arg[5] != "q") {
        cout << "not a valid selection" << endl;
        return 1;
    }
    else if (slct1.arg[5] == slct1.arg[0]) {
        cout << slct1.arg[5] << " was already declared to be a constant" << endl;
        return 1;
    }


    if(slct1.arg[2]!="peaktime" && slct1.arg[2]!="gain" && slct1.arg[2]!="channel" && slct1.arg[2]!="capacitance" && slct1.arg[2]!="temperature"){
        cout << "not a valid selection" << endl;
        return 1;
    }else if(slct1.arg[0]==slct1.arg[2]){
        cout << slct1.arg[2] << " was declared to be a constant" << endl;
        return 1;
    }else if((slct1.arg[2]=="capacitance" || slct1.arg[2]=="temperature") && slct1.arg[5]=="q"){
        cout << "To have capacitance or temperature on x axis you must have two constant variables" << endl;
        return 1;
    }
    
    if(slct1.arg[5]=="q" && slct1.arg[2]!="capacitance"){ //
        if(slct1.arg[3]!="peaktime" && slct1.arg[3]!="gain" && slct1.arg[3]!="channel" && slct1.arg[3]!="capacitance"){
            cout << "not a valid selection" << endl;
            return 1;
        }else if(slct1.arg[2]==slct1.arg[3]){
            cout << slct1.arg[3] << " is already placed on x axis" << endl;
            return 1;
        }else if(slct1.arg[0]==slct1.arg[3] || slct1.arg[5]==slct1.arg[3]){
            cout << slct1.arg[3] << " was declared to be a constant" << endl;  
            return 1;             
        }else if(slct1.arg[3] == "capacitance"){
            cout << "To have capacitance on z axis you must have two constant variables." << endl;
            return 1;
        }
    }else if(slct1.arg[5]!="q" && slct1.arg[2]!="capacitance"){
        if(slct1.arg[3] != "capacitance"){
            slct1.arg[3]=slct1.arg[5];
        }
    }else if(slct1.arg[5]!="q" && slct1.arg[2]=="capacitance"){
        if(slct1.arg[3]!="peaktime" && slct1.arg[3]!="gain" && slct1.arg[3]!="channel" && slct1.arg[3]!="capacitance"){
            cout << "not a valid selection" << endl;
            return 1;
        }else if(slct1.arg[2]==slct1.arg[3]){
            cout << slct1.arg[3] << " is already placed on x axis" << endl;
            return 1;
        }else if(slct1.arg[0]==slct1.arg[3] || slct1.arg[5]==slct1.arg[3]){
            cout << slct1.arg[3] << " was declared to be a constant" << endl;  
            return 1;             
        }
    }
        

    if(slct1.arg[1]!="pulse_ampl" && slct1.arg[1]!="enc_acqon" && slct1.arg[1]!="enc_acqoff" && slct1.arg[1]!="rms_acqon" && 
    slct1.arg[1]!="rms_acqoff" && slct1.arg[1] != "enc" && slct1.arg[1] != "rms"  && slct1.arg[1] != "temperature"  && slct1.arg[1] != "step"){
            cout << "not a valid selection" << endl;
            return 1;
    }else if((slct1.arg[1] == "enc" || slct1.arg[1] == "rms") && slct1.arg[3]=="capacitance") {
            // previously chosen z axis becomes a constant
            cout << "Capacitance is on z axis, can't plot two types of enc/rms" << endl;
            return 1;
    }
    

    slct1 = StringToNum(slct1);
    slct1 = PrintSelection(slct1);
    
    cout << slct1.arg_int[0] << '\t' << slct1.arg_int[1] << '\t' << slct1.arg_int[2] << '\t' << slct1.arg_int[3] << '\t' << slct1.arg_int[4] << endl;

    //vector<z> z1; //this vector contains objects of type z, as in, peaktime1, peaktime2 or gain1, gain2, or chn1, chn2...etc.
    //read result file and save values
     z read; 
     int j = 0;
     vector<z> z1;

     switch(slct1.arg_int[3]){
        case 0:
            for(int i=0;i<6;i++){
                z1.push_back(z());
            }
            break;
        case 1:
            for(int i=0;i<4;i++){
                z1.push_back(z());
            }
            break;
        case 2:
            for(int i=0;i<8;i++){
                z1.push_back(z());
            }
            break;
        case 3:
            if((slct1.arg_int[0]==0 && slct1.arg_int[4]>61) || (slct1.arg_int[5]==0 && slct1.arg_int[6]>61)){
             for(int i=0;i<9;i++){
                z1.push_back(z());
             }
            }else{
                z1.push_back(z());
                z1.push_back(z());
            }
            break;
     }
        
        //Open data file
	    string dataFile = "results.txt";
	    fstream myfile(dataFile);
	    if(myfile){
		    //cout << "Successfully opened data file." << endl;
	    }
	    else {
		    cout << "Error opening data file." << endl;
            return 1;
	    }

        while (!myfile.eof()) {
		myfile >> read.vmm_id >> read.chan >> read.gain >> read.peaktime >>  read.cap  >> read.rms1 >> read.rms2 >> read.rms1_er >> read.rms3 >> read.rms4 >> read.rms3_er >> 
        read.pa >> read.pa_er >> read.enc1 >> read.enc1_er >> read.enc2 >> read.enc2_er >> read.acq_contr >> read.temp >> read.delta_u >> 
        read.p_dac >> read.p_dac_er; // use tab spaced columns
       // cout << "vmm:" << read.vmm_id << " chan:" << read.chan << " gain:" << read.gain << " pt:" << read.peaktime << 
         //                           " read/rms_acqoff:" << read.rms1 << " read/rms acqoff error: " << read.rms1_er << endl;
        //cout << "filling vectors..." << endl;
        z1 = FillVectors(slct1,read,z1);
        //cout << "--------------------------------------------------------------------------------------------" << endl;






        j++;
        //if(j>=2048){
       // break;
       // }
        
	}
        myfile.close();
  cout << "done" << endl;
  PlotVectors(slct1,read,z1);
  return 0;
}

void plotdata(){
    int directory = system("mkdir last_result");
    selection slct2;
    slct2.arg[0]= "peaktime"; //constant 1
    slct2.arg[4]= "0"; //constant 1 value
    slct2.arg[1]= "enc_acqon"; // y axis
    slct2.arg[2]= "channel"; //x axis
    slct2.arg[3]= "gain";   //z axis
    slct2.arg[5]= "q"; //optional second constant, "q" if no constant
    //slct2.arg[6]= "";
    ReadData(slct2);

    selection slct3;
    slct3.arg[0]= "gain"; //constant 1
    slct3.arg[4]= "7"; //constant 1 value
    slct3.arg[1]= "enc_acqon"; // y axis
    slct3.arg[2]= "channel"; //x axis
    slct3.arg[3]= "peaktime";   //z axis
    slct3.arg[5]= "q"; //optional second constant, "q" if no constant
    //slct3.arg[6]= "7";
    ReadData(slct3);


    /*

    selection slct3;
    slct3.arg[0]= "channel"; //constant 1
    slct3.arg[4]= "62"; //constant 1 value
    slct3.arg[1]= "enc_acqon"; // y axis
    slct3.arg[2]= "capacitance"; //x axis
    slct3.arg[3]= "gain";   //z axis
    slct3.arg[5]= "peaktime"; //optional second constant, "q" if no constant
    slct3.arg[6]= "0";
    ReadData(slct3);

    selection slct4;
    slct4.arg[0]= "channel"; //constant 1
    slct4.arg[4]= "63"; //constant 1 value
    slct4.arg[1]= "enc_acqon"; // y axis
    slct4.arg[2]= "capacitance"; //x axis
    slct4.arg[3]= "peaktime";   //z axis
    slct4.arg[5]= "gain"; //optional second constant, "q" if no constant
    slct4.arg[6]= "7";
    ReadData(slct4);

    selection slct5;
    slct5.arg[0]= "channel"; //constant 1
    slct5.arg[4]= "63"; //constant 1 value
    slct5.arg[1]= "enc_acqon"; // y axis
    slct5.arg[2]= "capacitance"; //x axis
    slct5.arg[3]= "gain";   //z axis
    slct5.arg[5]= "peaktime"; //optional second constant, "q" if no constant
    slct5.arg[6]= "0";
    ReadData(slct5); */

   /* selection slct3;
    slct3.arg[0]= "channel"; //constant 1
    slct3.arg[4]= "12"; //constant 1 value
    slct3.arg[1]= "enc_acqon"; // y axis
    slct3.arg[2]= "gain"; //x axis
    slct3.arg[3]= "peaktime";   //z axis
    slct3.arg[5]= "q"; //optional second constant, "q" if no constant
    //slct3.arg[6]= "";
    ReadData(slct3);

    selection slct4;
    slct4.arg[0]= "peaktime"; //constant 1
    slct4.arg[4]= "0"; //constant 1 value
    slct4.arg[1]= "enc_acqon"; // y axis
    slct4.arg[2]= "gain"; //x axis
    slct4.arg[3]= "channel";   //z axis
    slct4.arg[5]= "q"; //optional second constant, "q" if no constant
    //slct4.arg[6]= "";
    //in case channel is on z axis -> specify 6 channels
    slct4.chn_id[0]=9;
    slct4.chn_id[1]=17;
    slct4.chn_id[2]=26;
    slct4.chn_id[3]=34;
    slct4.chn_id[4]=48;
    slct4.chn_id[5]=60;
    ReadData(slct4);

    selection slct5;
    slct5.arg[0]= "channel"; //constant 1
    slct5.arg[4]= "0"; //constant 1 value
    slct5.arg[1]= "enc_acqon"; // y axis
    slct5.arg[2]= "gain"; //x axis
    slct5.arg[3]= "peaktime";   //z axis
    slct5.arg[5]= "q"; //optional second constant, "q" if no constant
    //slct5.arg[6]= "";
    ReadData(slct5); */
}

