/*Cluster module - contains the general functions for the cluster,such 
 * as the initialiser, exoltuion modules, and output functionality*/

#include "../emacss.h"

static double a2=0.2,a3=0.3,a4=0.6,a5=1.0,a6=0.875,b21=0.2,
        b31=3.0/40.0,b32=9.0/40.0,b41=0.3,b42 = -0.9,b43=1.2,
	b51 = -11.0/54.0, b52=2.5,b53 = -70.0/27.0,b54=35.0/27.0,
	b61=1631.0/55296.0,b62=175.0/512.0,b63=575.0/13824.0,
	b64=44275.0/110592.0,b65=253.0/4096.0,c1=37.0/378.0,
	c3=250.0/621.0,c4=125.0/594.0,c6=512.0/1771.0,
	dc5 = -277.00/14336.0;
  double dc1=c1-2825.0/27648.0,dc3=c3-18575.0/48384.0,
	 dc4=c4-13525.0/55296.0,dc6=c6-0.25;

double node::E_calc(){                                //Equation (1) AG2012
  return -kappa*pow(N*mm.nbody,2)/r.nbody;
}

double node::r_jacobi(){                            //Equation (18) AG2012
  double rj = 0;
  if (galaxy.type == 1)
	rj = pow((N*mm.nbody)/(3.0*galaxy.M.nbody),(1.0/3.0))*galaxy.R.nbody;
  else if (galaxy.type == 2)
	rj = pow((N*mm.nbody)/(2.0*galaxy.M.nbody),(1.0/3.0))*galaxy.R.nbody;
  return rj;
}

double node::trh(){                                  //Equation (4) AG2012
return 0.138*sqrt(N*pow(r.nbody,3)/(mm.nbody))/(log(gamma*N));
}

double node::t_crj(){                                  //Crossing time at rj
 return pow((4.0*M_PI*pow(rj.nbody,3))/(3.0*N*mm.nbody),1.0/2.0);
}

double node::step_min(){                           //Ensures progress!
   return 1.0/(1.0/(frac*t_rhp.nbody)+1.0/(frac*time.nbody));
}

void node::evolve(stellar_evo se,dynamics dyn){
  /*Does what it says in the title  - evolves by rk4 kernel.*/
  static double duplicate_nbody[12]; 
  static double dr1[12],dr2[12],dr3[12],dr4[12],dr5[12],dr6[12], errs[12];
  static double safety = 0.9, shrink = -0.25, grow = -0.2;
  double error, step_test;
  
  convert(se);                              //Checks all quantities calculated

  if (time.nbody == 0) tstep = 0.1;

  if (time.nbody+tstep > out_time.nbody)     //incase Completing
	tstep = 1.001*(out_time.nbody-time.nbody);
  
  for (int i=0;i<12;i++) duplicate_nbody[i] = *nbody[i]; //Backup
  for (;;){
    for (int i=0;i<12;i++) *nbody[i] = duplicate_nbody[i];
    solve_odes(dr1,se,dyn);
    convert(se);
  
    for (int i=0;i<12;i++) 
      *nbody[i]=duplicate_nbody[i]+tstep*(b21*dr1[i]);
    solve_odes(dr2,se,dyn);
    convert(se);
   
    for (int i=0;i<12;i++) 
      *nbody[i]=duplicate_nbody[i]+tstep*(b31*dr1[i]+b32*dr2[i]);
    solve_odes(dr3,se,dyn);
    convert(se);
  
    for (int i=0;i<12;i++) 
      *nbody[i]=duplicate_nbody[i]+tstep*(b41*dr1[i]+b42*dr2[i]+b43*dr3[i]);
    solve_odes(dr4,se,dyn);
    convert(se);
  
    for (int i=0;i<12;i++) 
      *nbody[i]=duplicate_nbody[i]+tstep*(b51*dr1[i]+b52*dr2[i]+b53*dr3[i]+b54*dr4[i]);
    solve_odes(dr5,se,dyn);
    convert(se);
  
    for (int i=0;i<12;i++) 
      *nbody[i]=duplicate_nbody[i]+tstep*(b61*dr1[i]+b62*dr2[i]+b63*dr3[i]+b64*dr4[i]+b65*dr5[i]);
    solve_odes(dr6,se,dyn);
    convert(se); 
  
    for (int i=0;i<12;i++){
      *nbody[i]=duplicate_nbody[i]+tstep*(c1*dr1[i]+c3*dr3[i]+c4*dr4[i]+c6*dr6[i]);
      errs[i] = tstep*(dc1*dr1[i]+dc3*dr3[i]+dc4*dr4[i]+dc5*dr5[i]+dc6*dr6[i]);
    }
    convert(se); 
    
   if (time.nbody+tstep > out_time.nbody) break;
    
   error = 0;
   for (int i=0;i<12;i++)                   //Checks internal error calculated
       error = fmax(error, fabs(errs[i] / *nbody[i]))*2.5; 
   if (error<= 1.0) break;                 //If error < 20% accept step
   if (tstep < step_min()) break;          //Minimum stepsize - must proceed!
   step_test = safety*tstep*pow(error,shrink);
   tstep=(tstep >= 0.0 ? fmax(step_test,0.1*tstep) : fmin(step_test,0.1*tstep)); 	
   
  }
  if (tstep > step_min()){
    if (error >  1.89e-4) tstep = safety*tstep*pow(error,grow);
    else tstep = 5.0*tstep;
  } 
}

void node::solve_odes(double dvdt[],stellar_evo se,dynamics dyn){
/*Essentially a wrapper function; wraps the solving of all requsite differential
 equations into one function. Calls to a variety of dynamics and se functions.*/	
   
    dvdt[0] = 1;
    dvdt[1] = 0;
    dvdt[2] = dyn.dNdt();
    dvdt[3] = dyn.dmmdt();
    dvdt[4] = dyn.drdt();
    dvdt[5] = 0;
    dvdt[6] = 0;
    dvdt[7] = dyn.dkdt();
    dvdt[8] = dyn.dtrhdt();
    dvdt[9] = dyn.dtrhpdt();
    dvdt[10] = se.dmsegdt();
    dvdt[11] = se.dmsedt();
}

void node::convert(stellar_evo se){
 /*Housekeeping function. Keeps n-body and real units aligned when called.
  Also calls functions to determine the quantities that need to be redefined*/
  
    E.nbody = E_calc(); E.real = E.nbody*pow(M_star,2)/R_star;
    t_relax.nbody = trh(); t_relax.Myr = t_relax.nbody*T_star;
    t_rhp.nbody = t_relax.nbody/se.zeta(); t_rhp.Myr = t_rhp.nbody*T_star;
    rj.nbody = r_jacobi(); rj.pc = rj.nbody*R_star;
    tcrj.nbody = t_crj(); tcrj.Myr = tcrj.nbody*T_star;
    rhrj = r.nbody/rj.nbody;
    
    *real[0] = *nbody[0]*T_star;
    *real[2] = *nbody[2];
    *real[3] = *nbody[3]*M_star;
    *real[4] = *nbody[4]*R_star;
    *real[7] = *nbody[7];
    *real[8] = *nbody[8];
    *real[9] = *nbody[9];
    *real[10] = *nbody[10];
    *real[11] = *nbody[11]*M_star;
}