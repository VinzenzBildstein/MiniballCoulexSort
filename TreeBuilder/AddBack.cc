#ifndef __ADDBACK_CXX
#define __ADDBACK_CXX

#define PBINS 800
#define PRANGE 800
#define PMIN -1.0*PRANGE/PBINS
#define PMAX PRANGE+PMIN

#define GBINS 4000
#define GRANGE 4000
#define GMIN -1.0*GRANGE/GBINS
#define GMAX GRANGE+GMIN

#define ELBINS 2000
#define ELRANGE 2000
#define ELMIN -1.0*ELRANGE/ELBINS
#define ELMAX ELRANGE+ELMIN

#ifndef AddBack_hh
# include "AddBack.hh"
#endif

void AddBack::ClearEvt() {
	
	gen_array.clear();
	gtd_array.clear();
	clu_array.clear();
	cid_array.clear();
	sid_array.clear();
	sen_array.clear();
	
	vector<float>().swap(gen_array);
	vector<long long>().swap(gtd_array);
	vector<unsigned short>().swap(clu_array);
	vector<unsigned short>().swap(cid_array);
	vector<unsigned short>().swap(sid_array);
	vector<float>().swap(sen_array);

	return;
	
}

void AddBack::MakeGammaRays( bool addback, bool reject ) {
	
	// Crap segments list (i.e. those that need to be vetoed)
	// Counting from 0 to 167, i.e. including cores - clu*21 + core*7 + seg
	//dead_segments.push_back( 106 ); // clu 5, core 0, seg 1 = 18A1
	//dead_segments.push_back( 107 ); // clu 5, core 0, seg 2 = 18A2

	
	// ------------------------------------------------------------------------ //
	// Only DGF events
	// ------------------------------------------------------------------------ //
	for( unsigned int j = 0; j < event->NumberOfDgfs(); j++ ) {
		
		reject_evt = false;
		ab_evt = false;
		ab_mul = 0;
		veto_gamma = false;
		dgf_num = event->Dgf(j)->ModuleNumber();
		dgf_ch  = event->Dgf(j)->Channel();
		dgf_en  = event->Dgf(j)->Energy();
		dgf_t   = event->Dgf(j)->Time();
		
		// Miniball, this is where we do real gammas and addback
		if( 0 <= dgf_num && dgf_num < 48 && 0 <= dgf_ch && dgf_ch < 4 ) {
			
			GammaEnergy = Cal->DgfEnergy( dgf_num, dgf_ch, dgf_en );
			
			if( dgf_num % 2 == 0 && dgf_ch < 3 ) { // cores plus seg1 and seg2
				
				E_gam_seg[dgf_num/6][dgf_num%6/2][dgf_ch]->Fill( dgf_en );
				E_gam_seg_cal[dgf_num/6][dgf_num%6/2][dgf_ch]->Fill( GammaEnergy );
				if( dgf_ch == 0 ) E_gam->Fill( GammaEnergy );

			} // even DGF number
			
			else {
				
				E_gam_seg[dgf_num/6][dgf_num%6/2][dgf_ch+3]->Fill( dgf_en );
				E_gam_seg_cal[dgf_num/6][dgf_num%6/2][dgf_ch+3]->Fill( GammaEnergy );
				
			} // odd DGF number
			
			// Look for a core event
			if( dgf_ch == 0 && dgf_num % 2 == 0 ) {
				
				MaxSegClu = dgf_num / 6;
				MaxSegCore = dgf_num % 6 / 2;
				MaxSegId = 0; // initialise as core (if no segment hit (dead), use core!)
				
				// Check for highest energy segment in same detector
				for( unsigned int k = 0; k < event->NumberOfDgfs(); k++ ) {
					
					dgf_num2 = event->Dgf(k)->ModuleNumber();
					dgf_ch2  = event->Dgf(k)->Channel();
					dgf_en2  = event->Dgf(k)->Energy();
					dgf_t2   = event->Dgf(k)->Time();
					
					// We don't care if it's the same event
					if( k == j ) continue;
					
					// Make sure it's a Miniball DGF
					if( dgf_num2 < 0 || dgf_num2 > 47 ) continue;
					
					// Get global segment number (0-167)
					gSeg = ( dgf_num2 / 2 ) * 7;
					if( dgf_num2 % 2 == 0 ) gSeg += dgf_ch2;
					else gSeg += dgf_ch2+3;
					
					// Is it a crap segment?
					for( unsigned int ds = 0; ds < dead_segments.size(); ds++ ) {
						
						if( gSeg == dead_segments[ds] ) {
							
							veto_gamma = true;
							
						}
						
					}
					
					// Skip if a different detector
					if( dgf_num2 != dgf_num && dgf_num2 != dgf_num + 1 ) continue;
					
					GammaEnergy2 = Cal->DgfEnergy( dgf_num2, dgf_ch2, dgf_en2 );
					
					// Test maximum energy segment
					if( GammaEnergy2 < MaxSegEnergy ) continue;
					
					// Reassign energy and id
					MaxSegEnergy = GammaEnergy2;
					if( dgf_num2 % 2 == 0 ) MaxSegId = dgf_ch2;
					else MaxSegId = dgf_ch2+3;
					
				} // k
				
				// Found highest energy segment
				
				// Do the veto of crap segments
				if( veto_gamma ) continue;
				
				// Check previous gammas
				for( unsigned int l = 0; l < gen_array.size(); l++ ) {
					
					if( gen_array[l] < GammaEnergy+0.2 && gen_array[l] > GammaEnergy-0.2 &&
					   sid_array[l] == MaxSegId ) continue; // same event
					
					// Do the addback if same cluster
					if( clu_array[l] == MaxSegClu && addback ) {
						
						gen_array[l] += GammaEnergy;
						ab_evt = true;
						ab_mul++;
						
						if( sen_array[l] < MaxSegEnergy ) {
							
							gtd_array[l] = dgf_t;
							clu_array[l] = MaxSegClu;
							cid_array[l] = MaxSegClu*3 + MaxSegCore;
							sid_array[l] = MaxSegId;
							sen_array[l] = MaxSegEnergy;
							
						}

					} // addback
					
					// Reject/suppress if same cluster
					if( clu_array[l] == MaxSegClu && reject ) {
							
						gen_array.erase( gen_array.begin() + l );
						gtd_array.erase( gtd_array.begin() + l );
						clu_array.erase( clu_array.begin() + l );
						cid_array.erase( cid_array.begin() + l );
						sid_array.erase( sid_array.begin() + l );
						sen_array.erase( sen_array.begin() + l );
						
						reject_evt = true;
						l--;
						
					} // reject
					
				} // previous gammas
				
				if( ab_evt || reject_evt ) continue; // get next gamma
				hABmult->Fill( ab_mul );
				
				gen_array.push_back( GammaEnergy );
				gtd_array.push_back( dgf_t );
				clu_array.push_back( MaxSegClu );
				cid_array.push_back( MaxSegClu*3 + MaxSegCore );
				sid_array.push_back( MaxSegId );
				sen_array.push_back( MaxSegEnergy );
				
			} // core event
			
			// Reset
			MaxSegEnergy = -99.;
			MaxSegClu = -1;
			MaxSegCore = -1;
			MaxSegId = -1;
			
		}

		if( dgf_num == 53 && dgf_ch >= 0 && dgf_ch < 2 ) {
			
			GammaEnergy = Cal->DgfEnergy( dgf_num, dgf_ch, dgf_en );
			E_BeamDump[dgf_ch]->Fill( GammaEnergy );
			T_BeamDump[dgf_ch]->Fill( (dgf_t)/40000000 );
			
			for( unsigned int k = 0; k < event->NumberOfDgfs(); k++ ) {
				
				if( k == j ) continue;
				dgf_num2 = event->Dgf(k)->ModuleNumber();
				dgf_ch2  = event->Dgf(k)->Channel();
				dgf_en2  = event->Dgf(k)->Energy();
				dgf_t2   = event->Dgf(k)->Time();
				
				if( dgf_num2 != 53 ) continue;
				if( dgf_ch != 0 || dgf_ch2 != 1 ) continue;
				
				GammaEnergy2 = Cal->DgfEnergy( dgf_num2, dgf_ch2, dgf_en2 );
				
				tdiff_BD->Fill( dgf_t-dgf_t2 );
				
				if( TMath::Abs( dgf_t-dgf_t2 ) < 999999999. )
					
					bd_bd->Fill( GammaEnergy, GammaEnergy2 );
				
			}
			
		}
		
	}
	
	return;
	
}

void AddBack::MakeElectrons() {

	// SPEDE is in the 5th ADC
	for( unsigned int i = 0; i < subevent->Size(); i++ ) {
		
		adc_ch = subevent->AdcChannel(i);
		adc_en = subevent->AdcValue(i);
		
		ElectronEnergy = Cal->AdcEnergy( 4, adc_ch, adc_en );
		
		// STM-16 one
		if( adc_ch < 12 ) {
			
			E_spede_seg[adc_ch]->Fill( adc_en );
			E_spede_seg_cal[adc_ch]->Fill( ElectronEnergy );
			E_spede->Fill( ElectronEnergy );
			
			gen_array.push_back( ElectronEnergy );
			gtd_array.push_back( adc_t );
			clu_array.push_back( 8 );
			cid_array.push_back( 0 );
			sid_array.push_back( adc_ch );
			sen_array.push_back( 0 );
			
		}
		
		// STM-16 two
		else if( adc_ch > 15 && adc_ch < 28 ) {
			
			E_spede_seg[adc_ch-4]->Fill( adc_en );
			E_spede_seg_cal[adc_ch-4]->Fill( ElectronEnergy );
			E_spede->Fill( ElectronEnergy );
			
			gen_array.push_back( ElectronEnergy );
			gtd_array.push_back( adc_t );
			clu_array.push_back( 8 );
			cid_array.push_back( 0 );
			sid_array.push_back( adc_ch-4 );
			sen_array.push_back( 0 );
			
		}
		
	} // k
	
	return;
	
}

#endif