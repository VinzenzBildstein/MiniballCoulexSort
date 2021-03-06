// Sorting for Miniball Coulex data and building histograms
// Cuts are done in doppler.hh using function, doppler::Cut()
// Liam Gaffney (liam.gaffney@cern.ch) - 26/10/2016

#define g_clx_cxx

// Select settings by uncommenting one of the following: 
//#define TWOPART 		// define to plot every 2p combination
//#define PHICAL  		// define to plot different CD rotation offsets
//#define GEANG			// define for plotting Ge angles per cluster
#define MBGEOMETRY  		// define to overwrite MB angles with MBGeometry routine
#define SPEDEGEOMETRY	// define to overwrite Spede angles with SpedeGeometry routine

#ifdef PHICAL
# define PHI_STEP_WIDTH 1 
# define PHI_NSTEPS 21 // try to use an odd number
#endif

#ifndef __MBGEOMETRY_HH__
# include "MBGeometry.hh"
#endif
#ifndef __SpedeGeometry_HH__
# include "SpedeGeometry.hh"
#endif
#ifndef hist_hh
# include "hists.hh"
#endif
#ifndef g_clx_hh
# include "g_clx.hh"
#endif

void g_clx::Loop( string outputfilename ) {

	if( fChain == 0 ) return;

	// Output file name	
	TFile *out = new TFile( outputfilename.c_str(), "RECREATE" );

	// Fit stopping power curves from the srim output files
	// Comment out to use the default parameters in doppler.hh
	if( !doppler::stoppingpowers( ZP, ZT, AP, AT, "BT" ) || // beam in target
			!doppler::stoppingpowers( ZP, ZT, AP, AT, "TT" ) || // target in target
			//!doppler::stoppingpowers( ZP, ZT, AP, AT, "BS" ) || // beam in silicon
			!doppler::stoppingpowers( ZP, ZT, AP, AT, "TS" )	// target in silicon
	  ) return;

	// Ratio of prompt and random time windows
	// Alternatively, normalisation of beta-decay lines
	float bg_frac = -1.0;
	float del_frac = -0.85;

	// Test if it's an electron or gamma
	bool electron;

	// Include errors on histograms (required for correct bg subtraction)
	TH1::SetDefaultSumw2();

	// Declare the histograms here and initialise!
	hists h;
	h.Initialise();

	// Particle-particle time difference (from tppdiff)
	h.Set_ppwin(300.);

	// Maximum strip number where recoils and projectiles are separable
	// Applicable mostly for inverse kinematics. To turn off, set to 16
	h.Set_maxrecoil(16);

	// Minimum strip number to define high CoM recoils
	// For normal kinematics, this is likely to be the limit of the safe condition
	// i.e. strips that are >= minrecoil are unsafe. Only low CoM solution is safe
	h.Set_minrecoil(0);

	// New angles defined by Miniball geometry
#ifdef MBGEOMETRY

	//ifstream angfile;

	double clu_r[8] = { 135, 135, 135, 135, 135, 135, 135, 135 };
	double clu_theta[8] = { 136.7, 48.8, 48.8, 136.7, 135., 42.8, 135., 42.8 };
	double clu_phi[8] = { 129.5, 56.4, 132.4, 52.9, 236.6, 235., 310.5, 319. };
	double clu_alpha[8] = { 320., 300., 310., 310., 290., 240., 60., 120. }; 

	double new_theta[8][3][7]; // cluster, crystal segment
	double new_phi[8][3][7];

	MBGeometry mbg;	
	for( int i = 0; i < 8; i++ ) { // loop over clusters

		mbg.SetupCluster( clu_theta[i], clu_phi[i], clu_alpha[i], clu_r[i] );

		for( unsigned int j = 0; j < 3; j++ ) { // loop over cores

			new_theta[i][j][0] = mbg.GetCoreTheta(j) * TMath::DegToRad();
			new_phi[i][j][0] = mbg.GetCorePhi(j) * TMath::DegToRad();
			//cout << new_phi[i][j][0]*TMath::RadToDeg() << " ";	
			for( int k = 0; k < 6; k++ ) { // loop over segments

				new_theta[i][j][k+1] = mbg.GetSegTheta(j,k) * TMath::DegToRad();
				new_phi[i][j][k+1] = mbg.GetSegPhi(j,k) * TMath::DegToRad();

			}

		}

	}

#endif

	// New angles defined by Spede geometry
#ifdef SPEDEGEOMETRY
	double spede_r = SPEDEDIST;
	double spede_alpha = 0.0;

	double spede_theta[24];
	double spede_phi[24];

	SpedeGeometry spg;	
	spg.SetupSpede( spede_r, spede_alpha );
	for( unsigned int j = 0; j < 24; j++ ) { // loop over segments

		spede_theta[j] = spg.GetSpedeTheta(j) * TMath::DegToRad();
		spede_phi[j] = spg.GetSpedePhi(j) * TMath::DegToRad();
	}

#endif

	float rad, phi; // for cdpic

	// Loop over events 
	cout << "Looping over events...\n";
	Int_t nbytes = 0, nb = 0;
	Int_t skipFactor =1;
	for( Long64_t jentry=0; jentry<fChain->GetEntries()/skipFactor; jentry++ ) {	

		Long64_t ientry = LoadTree(jentry);

		if( ientry < 0 ) break;

		nb = fChain->GetEntry(jentry);   nbytes += nb;

		if( jentry%5000==0 ){
			cout << " " << jentry << "/" << fChain->GetEntries() << "  (" << jentry*100./fChain->GetEntries() << "%)    \r";
			cout << flush;
		}

		// Is it an electron or gamma?
		if( cluid < 8 ) electron = false;
		else if( cluid == 8 ) electron = true;
		else break; // shouldn't be anything else

		// Overwrite angles from tree with new angles
#ifdef MBGEOMETRY
		if( !electron ) { // check if it's Miniball

			tha = new_theta[cluid][cid%3][sid];
			pha = new_phi[cluid][cid%3][sid];

			for( unsigned int i = 0; i < gcor_gen.size(); i++ ){
		
				if( gcor_cluid[i] == 8 || gcor_sid[i] < 0 ) continue; // not Ge
				gcor_tha[i] = new_theta[gcor_cluid[i]][gcor_cid[i]%3][gcor_sid[i]];	// gcor_sid broken!
				gcor_pha[i] = new_phi[gcor_cluid[i]][gcor_cid[i]%3][gcor_sid[i]];		// gcor_sid broken!
			
			}
		
		}
#endif
#ifdef SPEDEGEOMETRY
		if( electron ) { // check if it's SPEDE/PAD

			if( cid == 0 ) { // spede

				tha = spede_theta[sid];
				pha = spede_phi[sid];

				for( unsigned int i = 0; i < gcor_gen.size(); i++ ){
			
					if( gcor_cluid[i] != 8 || gcor_sid[i] < 0 || gcor_sid[i] > 23 ) continue; // not spede/PAD
					gcor_tha[i] = spede_theta[gcor_sid[i]];		// gcor_sid broken!
					gcor_pha[i] = spede_phi[gcor_sid[i]];		// gcor_sid broken!
				
				}

			}
			
			else { // PAD

				tha = doppler::GetPTh( 10 );
				pha = doppler::GetPPhi( cid-1, 6 );

				for( unsigned int i = 0; i < gcor_gen.size(); i++ ){
			
					if( gcor_cluid[i] != 8 || gcor_sid[i] < 0 || gcor_sid[i] > 23 ) continue; // not spede/PAD
					gcor_tha[i] = spede_theta[gcor_sid[i]];		// gcor_sid broken!
					gcor_pha[i] = spede_phi[gcor_sid[i]];		// gcor_sid broken!
				
				}


			}

		}
#endif


		// Escape if angles are strange
		//if( gen >= 0 && ( tha < 0.0005 || pha < 0.0005 ) && !electron ){
		//	h.GeReject->Fill( cluid );
		//	continue;
		//}
		//h.GePass->Fill(cluid);

		// Paricle multiplicity
		if( pr_hits != 0 ) h.multp->Fill( pr_hits );
		else if( rndm_hits != 0 )  h.multr->Fill( rndm_hits );

		// Germanium angles
		if( !electron ) { // check if it's Miniball

			h.GeAng->Fill(tha*TMath::RadToDeg(),pha*TMath::RadToDeg());
#ifdef GEANG
			h.GeAng_clu[cluid]->Fill(tha*TMath::RadToDeg(),pha*TMath::RadToDeg());
#endif
		}

		// Loop over particle counter
		for( unsigned int i=0; i<pen.size(); i++ ){

			// Escape funny events if there are any
			if( det[i]<0 || det[i]>3 || ann[i]<0 || sec[i]<0 ) continue;

			// Fill particle-gamma time spectra
			h.tdiff->Fill(td[i]*25.);
			h.tdiffQ[det[i]]->Fill(td[i]*25.);
			for( unsigned int k=0; k<4; k++ )
				if( gen>h.tegate[k]-20 && gen<h.tegate[k]+20 )
					h.tdiffE[k]->Fill(td[i]*25.);

			// particle - particle time difference
			for( unsigned int j=i+1; j<pen.size(); j++ ) {
				h.tpp->Fill(td[i]*25.,td[j]*25.);
				h.tppdiff->Fill((td[i]-td[j])*25.);
				for( int k=0; k<2; k++ ){
					if( (det[i]==k && det[j]==k+2) || (det[j]==k && det[i]==k+2) ) {
						if( det[i]>det[j] ) h.tQQ[k]->Fill((td[j]-td[i])*25.);
						else h.tQQ[k]->Fill((td[i]-td[j])*25.);
					}
				}
			}

			// CD pic
			if( ann[i]>(-1) && ann.size()>0 ){
				rad=39-2*ann[i];
				rad+=gRandom->Rndm(1)*1.9;
				phi=(sec[i]-16.)*0.12+(det[i]*TMath::PiOver2());
				phi+=gRandom->Rndm(1)*0.114;
				if( sec[i]<29 && sec[i]>15 ) h.cdpic->Fill(rad*cos(phi),rad*sin(phi));
			}

			// Germanium angles vs. Silicon angles
			if( !electron ) {
#ifdef GEANG
				h.GeSiAng->Fill(tha*TMath::RadToDeg(),doppler::GetPTh(ann[i])*TMath::RadToDeg(),(pha-doppler::GetPPhi(det[i],sec[i]))*TMath::RadToDeg());
				h.GeSiAng_clu[cluid]->Fill(tha*TMath::RadToDeg(),doppler::GetPTh(ann[i])*TMath::RadToDeg(),(pha-doppler::GetPPhi(det[i],sec[i]))*TMath::RadToDeg());
#endif
			} // !electron
			
		} // END - Loop over particle counter

		// Check size of correlated gammas
		h.gcor_size->Fill( gcor_gen.size() );

		if( gcor_gen.size() > 0 ) h.FillGamGam0h( gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd );

		// Fill conditioned spectra

		// Condition on particle detection
		if( pr_hits==1 && rndm_hits==0 ) 
			h.Fill1h(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen[pr_ptr[0]], ann[pr_ptr[0]], sec[pr_ptr[0]], det[pr_ptr[0]], 1.0);

		else if( pr_hits==2 && rndm_hits==0 )
			h.Fill2h(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen, ann, sec, det, pr_ptr, td, 1.0);

		else if( rndm_hits==1 && pr_hits==0 && del_hits==0 ) 
			h.Fill1h(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen[rndm_ptr[0]], ann[rndm_ptr[0]], sec[rndm_ptr[0]], det[rndm_ptr[0]], bg_frac);

		else if( rndm_hits==2 && pr_hits==0 && del_hits==0 )			
			h.Fill2h(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen, ann, sec, det, rndm_ptr, td, bg_frac);

		else if( del_hits==1 && rndm_hits==0 ) 
			h.FillDelayed(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen[del_ptr[0]], ann[del_ptr[0]], sec[del_ptr[0]], det[del_ptr[0]], 1.0 );

		else if( del_hits==0 && rndm_hits==1 ) 
			h.FillDelayed(gen, tha, pha, gcor_gen, gcor_tha, gcor_pha, gcor_cluid, gcor_gtd, electron,
					 pen[del_ptr[0]], ann[del_ptr[0]], sec[del_ptr[0]], det[del_ptr[0]], del_frac );

	} // for (Long64_t jentry=0; jentry<fChain->GetEntries();jentry++)
	

	// Add spectra
	h.AddSpectra(bg_frac,del_frac);

	long nopart = (long)h.part->Integral();
	cout << "1-particle events = " << nopart << endl;

#ifdef PHICAL
	TCanvas *cphi;
	cphi = new TCanvas( "PhiCalibration", "Phi Calibration" );
	for( unsigned int i = 0; i < PHI_NSTEPS; i++ ) {

		h.phical_dcT[i]->SetLineColor(i+1);
		if( i == 0 ) h.phical_dcT[i]->Draw();
		else h.phical_dcT[i]->Draw("SAME");

	}
#endif

	out->Write();

}
