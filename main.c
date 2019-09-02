#include <processor_include.h>
#include <signal.h>
#include <string.h>
#include <filter.h>
#include <stdio.h>
#include <stdint.h>

#include "framework.h"

#define blockDivider 10
#define numOfBlock 100
#define lambdaCoeff 15
#define r0SpeechThreshold 0.00100
#define printToDatabase 0

static float mic1[DSP_BLOCK_SIZE];
static float mic2[DSP_BLOCK_SIZE];
static float mic3[DSP_BLOCK_SIZE];
static float mic4[DSP_BLOCK_SIZE];
static float allCoeff[numOfBlock][lambdaCoeff];
static float allR0[numOfBlock];



static int startProcessing = 0; // Done getting 2 second signal if 1. 
static int blockCounter = 0; // Counts from 0 to 300 blocks. 
static int partCounter = 0; // We use this to study which part we are on.
static float coeff[blockDivider][lambdaCoeff]; // 10x15 lambda matrix. 


static int isRecording = 0;

static int mute = 0;

static float database2[blockDivider][lambdaCoeff]=
{
{-0.692033187500000000000000000000, 0.802532500000000000000000000000, -0.208659156300000000000000000000, 0.300226500000000000000000000000, -0.138411781300000000000000000000, 0.171885531300000000000000000000, 0.290310406300000000000000000000, 0.215607078100000000000000000000, -0.056259987500000000000000000000, 0.052027462500000000000000000000, 0.008490532500000000000000000000, 0.108365150000000000000000000000, 0.058469681250000000000000000000, -0.049915500000000000000000000000, 0.000000000000000000000000000000},
	{-0.509695656300000000000000000000, 0.889493937500000000000000000000, 0.216004718800000000000000000000, 0.276990281300000000000000000000, 0.002211585156000000000000000000, 0.220070484400000000000000000000, 0.049448512500000000000000000000, 0.178458265600000000000000000000, 0.012126296250000000000000000000, 0.165087734400000000000000000000, 0.029169018750000000000000000000, 0.100349893800000000000000000000, -0.028659387500000000000000000000, 0.002558314063000000000000000000, 0.000000000000000000000000000000},
	{-0.473512937500000000000000000000, 0.909729312500000000000000000000, 0.393882343800000000000000000000, 0.439008531300000000000000000000, 0.096558000000000000000000000000, 0.260512328100000000000000000000, 0.008639673750000000000000000000, 0.141219796900000000000000000000, -0.022029254690000000000000000000, 0.099281525000000000000000000000, -0.046759909380000000000000000000, 0.070176087500000000000000000000, -0.041480493750000000000000000000, 0.060159431250000000000000000000, 0.000000000000000000000000000000},
	{-0.454496531300000000000000000000, 0.881521625000000000000000000000, 0.390075937500000000000000000000, 0.427041843800000000000000000000, 0.122768925000000000000000000000, 0.283703156300000000000000000000, 0.048813828130000000000000000000, 0.202044250000000000000000000000, -0.013077886250000000000000000000, 0.132608078100000000000000000000, -0.029211893750000000000000000000, 0.078791168750000000000000000000, -0.021795515630000000000000000000, 0.035655250000000000000000000000, 0.000000000000000000000000000000},
	{-0.464431281300000000000000000000, 0.893138437500000000000000000000, 0.387901343800000000000000000000, 0.428641781300000000000000000000, 0.091257237500000000000000000000, 0.236392906300000000000000000000, -0.006615336875000000000000000000, 0.150407890600000000000000000000, -0.007027821250000000000000000000, 0.108337300000000000000000000000, -0.041086896880000000000000000000, 0.056899706250000000000000000000, -0.006789833125000000000000000000, 0.045178337500000000000000000000, 0.000000000000000000000000000000},
	{-0.461854187500000000000000000000, 0.897154312500000000000000000000, 0.383739281300000000000000000000, 0.422706593800000000000000000000, 0.174382609400000000000000000000, 0.263042593800000000000000000000, 0.080618500000000000000000000000, 0.193584312500000000000000000000, 0.005452653750000000000000000000, 0.140106453100000000000000000000, 0.031306071880000000000000000000, 0.114298275000000000000000000000, 0.005512289375000000000000000000, 0.095059962500000000000000000000, 0.000000000000000000000000000000},
	{-0.454311218800000000000000000000, 0.872976250000000000000000000000, 0.375672968800000000000000000000, 0.453142500000000000000000000000, 0.128793554700000000000000000000, 0.283426187500000000000000000000, 0.045989137500000000000000000000, 0.181035828100000000000000000000, 0.033371975000000000000000000000, 0.151289609400000000000000000000, 0.014464947500000000000000000000, 0.088332568750000000000000000000, -0.001401842250000000000000000000, 0.061498762500000000000000000000, 0.000000000000000000000000000000},
	{-0.455548843800000000000000000000, 0.867077625000000000000000000000, 0.397933187500000000000000000000, 0.463234031300000000000000000000, 0.145364359400000000000000000000, 0.269646875000000000000000000000, -0.000687299500000000000000000000, 0.167600609400000000000000000000, -0.033731568750000000000000000000, 0.124223475000000000000000000000, 0.001725849875000000000000000000, 0.052102803130000000000000000000, -0.032575946880000000000000000000, 0.078346987500000000000000000000, 0.000000000000000000000000000000},
	{-0.459908312500000000000000000000, 0.885831750000000000000000000000, 0.398721750000000000000000000000, 0.488001187500000000000000000000, 0.129799585900000000000000000000, 0.256515406300000000000000000000, 0.034973759380000000000000000000, 0.206475015600000000000000000000, 0.018562176560000000000000000000, 0.120951625000000000000000000000, 0.015439621250000000000000000000, 0.091817287500000000000000000000, -0.006142315625000000000000000000, 0.093938875000000000000000000000, 0.000000000000000000000000000000},
	{-0.462459406300000000000000000000, 0.888291750000000000000000000000, 0.380660281300000000000000000000, 0.417704718800000000000000000000, 0.129434617200000000000000000000, 0.271399937500000000000000000000, 0.050257600000000000000000000000, 0.174689125000000000000000000000, -0.051905781250000000000000000000, 0.122827512500000000000000000000, -0.016653471880000000000000000000, 0.109319887500000000000000000000, -0.016927650000000000000000000000, 0.043171006250000000000000000000, 0.000000000000000000000000000000}
};
static float database1[blockDivider][lambdaCoeff]=
{
{-0.499410875000000000000000000000, 0.775394437500000000000000000000, -0.033136793750000000000000000000, 0.137480984400000000000000000000, -0.186707890600000000000000000000, -0.111380900000000000000000000000, -0.087040581250000000000000000000, 0.210833218800000000000000000000, 0.059334306250000000000000000000, 0.104332506300000000000000000000, 0.030290537500000000000000000000, 0.215748578100000000000000000000, 0.054306018750000000000000000000, 0.133144765600000000000000000000, 0.000000000000000000000000000000},
	{-0.459749593800000000000000000000, 0.878530312500000000000000000000, 0.370303312500000000000000000000, 0.425319875000000000000000000000, 0.135132671900000000000000000000, 0.243317828100000000000000000000, 0.054646937500000000000000000000, 0.180627656300000000000000000000, 0.024082000000000000000000000000, 0.105797487500000000000000000000, 0.047241796880000000000000000000, 0.086217318750000000000000000000, 0.012287122500000000000000000000, 0.074443000000000000000000000000, 0.000000000000000000000000000000},
	{-0.458631000000000000000000000000, 0.884836500000000000000000000000, 0.398024406300000000000000000000, 0.434632437500000000000000000000, 0.151067421900000000000000000000, 0.278098125000000000000000000000, 0.057455706250000000000000000000, 0.211311750000000000000000000000, 0.021833368750000000000000000000, 0.156034265600000000000000000000, 0.014510023750000000000000000000, 0.083967081250000000000000000000, 0.010657543750000000000000000000, 0.039112140630000000000000000000, 0.000000000000000000000000000000},
	{-0.452291968800000000000000000000, 0.878445125000000000000000000000, 0.387702468800000000000000000000, 0.449108437500000000000000000000, 0.142147781300000000000000000000, 0.260921390600000000000000000000, 0.044006265630000000000000000000, 0.176037500000000000000000000000, -0.002058961719000000000000000000, 0.130960750000000000000000000000, -0.016826079690000000000000000000, 0.091272831250000000000000000000, -0.000869840062500000000000000000, 0.067103393750000000000000000000, 0.000000000000000000000000000000},
	{-0.462949593800000000000000000000, 0.879093937500000000000000000000, 0.391941562500000000000000000000, 0.457445468800000000000000000000, 0.150285843800000000000000000000, 0.267663656300000000000000000000, 0.055396662500000000000000000000, 0.179800859400000000000000000000, -0.024833223440000000000000000000, 0.089507581250000000000000000000, -0.020216828130000000000000000000, 0.065665512500000000000000000000, -0.009116132500000000000000000000, 0.059809650000000000000000000000, 0.000000000000000000000000000000},
	{-0.454097750000000000000000000000, 0.888062437500000000000000000000, 0.385772281300000000000000000000, 0.435932937500000000000000000000, 0.139178906300000000000000000000, 0.277016843800000000000000000000, 0.028893440630000000000000000000, 0.175400562500000000000000000000, -0.002849612813000000000000000000, 0.149072796900000000000000000000, 0.023611435940000000000000000000, 0.079988718750000000000000000000, -0.013549538750000000000000000000, 0.067741737500000000000000000000, 0.000000000000000000000000000000},
	{-0.464896500000000000000000000000, 0.895190375000000000000000000000, 0.403208031300000000000000000000, 0.441304531300000000000000000000, 0.153258515600000000000000000000, 0.287554687500000000000000000000, 0.049920603130000000000000000000, 0.188073828100000000000000000000, 0.035977546880000000000000000000, 0.121944150000000000000000000000, 0.019640657810000000000000000000, 0.062942243750000000000000000000, -0.018306154690000000000000000000, 0.096423243750000000000000000000, 0.000000000000000000000000000000},
	{-0.462105718800000000000000000000, 0.893182000000000000000000000000, 0.372810281300000000000000000000, 0.433169218800000000000000000000, 0.145808718800000000000000000000, 0.294924562500000000000000000000, 0.092183425000000000000000000000, 0.185838531300000000000000000000, -0.009878136250000000000000000000, 0.129818859400000000000000000000, -0.025002500000000000000000000000, 0.097812831250000000000000000000, -0.019313350000000000000000000000, 0.032649896880000000000000000000, 0.000000000000000000000000000000},
	{-0.465060218800000000000000000000, 0.890536312500000000000000000000, 0.410144562500000000000000000000, 0.462848812500000000000000000000, 0.145601234400000000000000000000, 0.259371578100000000000000000000, 0.019704796880000000000000000000, 0.180323437500000000000000000000, -0.004711670313000000000000000000, 0.120574350000000000000000000000, 0.047637828130000000000000000000, 0.056408012500000000000000000000, 0.005117097500000000000000000000, 0.055095150000000000000000000000, 0.000000000000000000000000000000},
	{-0.467068718800000000000000000000, 0.894803375000000000000000000000, 0.407963031300000000000000000000, 0.423417093800000000000000000000, 0.132790406300000000000000000000, 0.271708000000000000000000000000, 0.040240559380000000000000000000, 0.187749453100000000000000000000, -0.007378427500000000000000000000, 0.124016350000000000000000000000, -0.037173468750000000000000000000, 0.074266300000000000000000000000, -0.025698423440000000000000000000, 0.057291331250000000000000000000, 0.000000000000000000000000000000}
};
static float database3[blockDivider][lambdaCoeff]=
{
{-0.597308687500000000000000000000, 0.918231500000000000000000000000, -0.239326796900000000000000000000, 0.224027109400000000000000000000, -0.034369125000000000000000000000, 0.177652046900000000000000000000, 0.082085875000000000000000000000, 0.106245012500000000000000000000, -0.101221793800000000000000000000, 0.102345743800000000000000000000, 0.058102081250000000000000000000, 0.036211803130000000000000000000, -0.032536025000000000000000000000, -0.055819937500000000000000000000, 0.000000000000000000000000000000},
	{-0.626160125000000000000000000000, 0.889555937500000000000000000000, -0.204520375000000000000000000000, 0.176824843800000000000000000000, 0.081818206250000000000000000000, 0.113028962500000000000000000000, -0.017800428130000000000000000000, 0.098764156250000000000000000000, -0.123983500000000000000000000000, 0.049781762500000000000000000000, 0.001799185625000000000000000000, 0.011870616250000000000000000000, -0.028785471880000000000000000000, 0.005324304375000000000000000000, 0.000000000000000000000000000000},
	{-0.508619437500000000000000000000, 0.945404375000000000000000000000, 0.092407225000000000000000000000, 0.177499062500000000000000000000, 0.026515906250000000000000000000, 0.218645625000000000000000000000, -0.054112343750000000000000000000, 0.108926112500000000000000000000, -0.075855062500000000000000000000, 0.003629606250000000000000000000, -0.056885456250000000000000000000, 0.011907261250000000000000000000, 0.001106419875000000000000000000, 0.001485269125000000000000000000, 0.000000000000000000000000000000},
	{-0.474622000000000000000000000000, 0.906610625000000000000000000000, 0.384068250000000000000000000000, 0.434722500000000000000000000000, 0.096386843750000000000000000000, 0.267102093800000000000000000000, 0.037594325000000000000000000000, 0.184937187500000000000000000000, 0.045115150000000000000000000000, 0.119536225000000000000000000000, 0.009311248750000000000000000000, 0.141810343800000000000000000000, -0.033671365630000000000000000000, 0.044915859380000000000000000000, 0.000000000000000000000000000000},
	{-0.461603718800000000000000000000, 0.882225000000000000000000000000, 0.398305281300000000000000000000, 0.427694937500000000000000000000, 0.135836125000000000000000000000, 0.246591406300000000000000000000, -0.009421054375000000000000000000, 0.186510718800000000000000000000, -0.029875515630000000000000000000, 0.102915125000000000000000000000, -0.018058462500000000000000000000, 0.090741175000000000000000000000, -0.026170073440000000000000000000, 0.063479281250000000000000000000, 0.000000000000000000000000000000},
	{-0.451588218800000000000000000000, 0.883248000000000000000000000000, 0.397391625000000000000000000000, 0.426161125000000000000000000000, 0.152605078100000000000000000000, 0.271220500000000000000000000000, 0.041455437500000000000000000000, 0.160957078100000000000000000000, -0.014343346250000000000000000000, 0.111195025000000000000000000000, -0.024374473440000000000000000000, 0.081171556250000000000000000000, -0.031478812500000000000000000000, 0.041248150000000000000000000000, 0.000000000000000000000000000000},
	{-0.456168031300000000000000000000, 0.876264875000000000000000000000, 0.401834968800000000000000000000, 0.459100250000000000000000000000, 0.113239262500000000000000000000, 0.278873187500000000000000000000, 0.034213156250000000000000000000, 0.190288625000000000000000000000, 0.010464890000000000000000000000, 0.102715943800000000000000000000, -0.003799889375000000000000000000, 0.085425325000000000000000000000, -0.048148565630000000000000000000, 0.068171100000000000000000000000, 0.000000000000000000000000000000},
	{-0.469177312500000000000000000000, 0.895477437500000000000000000000, 0.388526812500000000000000000000, 0.428350937500000000000000000000, 0.124873712500000000000000000000, 0.254321484400000000000000000000, -0.016456532810000000000000000000, 0.148756000000000000000000000000, -0.025613403130000000000000000000, 0.126381960900000000000000000000, -0.037741921880000000000000000000, 0.060270706250000000000000000000, -0.017807345310000000000000000000, 0.037890118750000000000000000000, 0.000000000000000000000000000000},
	{-0.463363750000000000000000000000, 0.886211812500000000000000000000, 0.370588812500000000000000000000, 0.451155562500000000000000000000, 0.143091640600000000000000000000, 0.256643234400000000000000000000, 0.024065859380000000000000000000, 0.156333937500000000000000000000, -0.039077221880000000000000000000, 0.100870268800000000000000000000, -0.030309037500000000000000000000, 0.052716800000000000000000000000, -0.036967196880000000000000000000, 0.042557650000000000000000000000, 0.000000000000000000000000000000},
	{-0.470729125000000000000000000000, 0.900902375000000000000000000000, 0.419302906300000000000000000000, 0.452010593800000000000000000000, 0.127790523400000000000000000000, 0.282252531300000000000000000000, 0.045612621880000000000000000000, 0.190705593800000000000000000000, 0.051159400000000000000000000000, 0.146475000000000000000000000000, 0.015814090630000000000000000000, 0.083721031250000000000000000000, -0.027916912500000000000000000000, 0.082848206250000000000000000000, 0.000000000000000000000000000000}
};



// Put in x and an empty r vector that will be filled afterwards. 
static void autoCorr(float x[], float r[], int mnum)
{
	int sizeOfX = DSP_BLOCK_SIZE;
	//printf("size: %d\n", sizeOfX);
	
	int i;
	int k;
	for (i = 0; i <= mnum; i++)
	{
		r[i] = 0;
		for (k = 0; k <= (sizeOfX-i); k++)
		{
		
			r[i] += x[k]*x[k+i];
		
	
		
		}
		r[i] /= (sizeOfX); // Unbiased autocorr = size -i, biased = size


	}
	allR0[blockCounter] = r[0]; // Puts all r[0] for 100 blocks = 2 seconds. 

}

static void resetCoeff() // sets 10x15 lambda matrix element to 0
{
	int i = 0;
	int k = 0;
	for (i = 0; i < blockDivider; i++)
	{
		for (k = 0; k < lambdaCoeff; k++)
		{
			coeff[i][k] = 0;	
		}
	}	
}

static void fill(float arrayToFill[], float valueInArray, int theSize) // Fill arrayToFill with float value valueInarray. Thesize is the length of array. 
{
	int sizeOfArray = theSize;
	//printf("SizeofArray: %d");
	int i;
	for (i = 0; i < sizeOfArray;i++)
	{
		arrayToFill[i] = valueInArray;
	}
}

static void levinson(float r[], int mnum)
{
	int i;
	int k;
	int n;
	float Ak[mnum+1];
	float Ek;
	fill(Ak, 0, mnum+1);
	Ak[0] = 1.0;
	Ek = r[0];
	for (i = 0; i < mnum; i++)
	{
		float lambda = 0.0;
		for (k = 0; k <= i; k++)
		{
			//printf("%.20f\n", lambda);
			lambda -= Ak[k]*r[i+1-k];
		}
	
	
		lambda /= Ek;
		for ( n = 0; n <= ( i + 1 ) / 2; n++ )
 		{
 			float temp = Ak[ i + 1 - n ] + lambda * Ak[ n ];
 			Ak[ n ] = Ak[ n ] + lambda * Ak[ i + 1 - n ];
 			Ak[ i + 1 - n ] = temp;
 		}
 		// UPDATE Ek

 		//coeff[partCounter][i] += lambda;
 		//printf("coeff: %.20f\n", lambda);
 		allCoeff[blockCounter][i] = lambda; // as blockcounter increase put all values inside. When voice not trigger it will just be inputted at place 0. 
 		Ek *= 1.0 - lambda * lambda;
 		//printf("Error (should decrease): %.20f\n", Ek);
 	}
 //	printf("New block \n");
 	
 	
 	
	
	

}


static void printAllCoeffAndR0() // print 100x15 lambda matrix. 
{
	//printf("Printing all coeff and R0\n");
	int i;
	int k;
	for (i = 0; i < numOfBlock; i++)
	{
		if (allR0[0] > 0)
		{
			//printf("RO: %.6f || %d\n", allR0[i], allR0[i]>r0SpeechThreshold);
		}
		for (k = 0; k < lambdaCoeff; k++)
		{
			if (allCoeff[i][k] > 0)
			{
			//printf("COEFF: %.30f\n", allCoeff[i][k]);
			}
		}	
	}
	printf("STop\n");
}

static void printCoeff() // print 10x15 lambda matrix.
{
	printf("Printing coeff\n");
	int i;
	int k;
	float sumCoeff = 0;
	for (i = 0; i < blockDivider; i++)
	{
		if(printToDatabase == 1) 
		{
			printf("\n{%.30f", coeff[i][0]);
		}
		for (k = 1; k < lambdaCoeff; k++)
		{
			if(printToDatabase == 1) 
			{
				printf(", %.30f", coeff[i][k]);
			}
			sumCoeff += coeff[i][k];
			if (coeff[i][k] > 1 || coeff[i][k] < -1)
			{
			
					//printf("%.30f || %d || %d\n", coeff[i][k], i, k);
				
			}
		}
		if(printToDatabase == 1) 
		{
			printf("},");
		}
			
	}
	printf("\nSTop sum: %.30f\n", sumCoeff);
}

float state[3];

static void prefilter(float x[],float xf[])    // prefiltern before starting. 
{
	int sizeOFX= DSP_BLOCK_SIZE;

	static const pm float  coeffs[5]={-0.95,1,0.75,-1.75,1}; 
	//fill(state,0, 3);
	biquad(x,xf,coeffs,state,sizeOFX,1);
	
	
}

static unsigned int matching() // Matching algoritmn
{
	float diff[3]={0,0,0};
	unsigned int i=0;
	unsigned int j=0;
	unsigned int voiceindex=9;
	float temp=0;

	for(i=0;i<3;i++)
	{
		for(j=0;j<lambdaCoeff;j++)
		{
			temp=coeff[i][j]-database1[i][j];
			if(temp<0)
			{temp=-temp;}
			diff[0]+=temp;
		}
	}
	for(i=0;i<3;i++)
	{
		for(j=0;j<lambdaCoeff;j++)
		{
			temp=coeff[i][j]-database2[i][j];
			if(temp<0)
			{temp=-temp;}
			diff[1]+=temp;
		}
	}

	for(i=0;i<3;i++)
	{
		for(j=0;j<lambdaCoeff;j++)
		{
			temp=coeff[i][j]-database3[i][j];
			if(temp<0)
			{temp=-temp;}
			diff[2]+=temp;
		}
	}
	printf("%f || %f || %f\n", diff[0], diff[1], diff[2]);

	//select the minimum
	if(diff[0]<diff[1]&&diff[0]<diff[2])
	{
		voiceindex=1;
	}
	else if(diff[1]<diff[0]&&diff[1]<diff[2])
	{
		voiceindex=2;
	}
	else
	{
		voiceindex=3;
	}
	
	if (diff[voiceindex-1]>40)//doesn't match any word in database,put threshold here
	{
		voiceindex=9;//9 means no match
	}
	return voiceindex;
}

void process(int sig) // Code starts running here when interrupt is called, we get the matrix values from here.
{
    int n;
   
	
    sample_t *u30 = dsp_get_audio_u30();    /* line 2 in without mic bias, no out */
    sample_t *u31 = dsp_get_audio_u31();    /* line 1 in with mic bias, no out */
    sample_t *u32 = dsp_get_audio_u32();    /* mic 1 and 2 in, headset out */
    sample_t *u33 = dsp_get_audio_u33();    /* mic 3 and 4 in, no out */
    
    
   

    for(n=0; n<DSP_BLOCK_SIZE; ++n) { // 320 block size
        mic1[n] = u32[n].right;
        mic2[n] = u32[n].left;
        mic3[n] = u33[n].right;
        mic4[n] = u33[n].left;
    }
  
    for(n=0; n<DSP_BLOCK_SIZE; ++n) {
        u32[n].right = mic1[n] + mic2[n] + mic3[n] + mic4[n]; // Added by me to configure the mics was divided before
        u32[n].left  = mic3[n] + mic4[n] + mic1[n] + mic2[n];
    }
    
    //printf("%.6f\n", mic1[100]);
     //printf("%d\n", blockCounter);
    #define mnum 14
	float r[mnum+1];
	static float xf[DSP_BLOCK_SIZE];
	// allCoeff[100][15];
	
   	prefilter(mic1,xf);
    autoCorr(xf, r, mnum);
	levinson(r, mnum);
   

    if (r[0] > r0SpeechThreshold && isRecording == 0)
    {
    	isRecording = 1;
    	printf("Recording voice\n");	
    	printf("Triggered on value: %.30f\n", r[0]);
    	
    }
    
    if (isRecording == 1)
    {
    	allR0[blockCounter] = r[0];
    	blockCounter++; // Done processing a block. we process 300 blocks = 6 seconds.
    	if (blockCounter == 100) // 2 s
    	{
    		isRecording = 0;
    		//printf("Recording stopped\n");
    		startProcessing = 1;
    		blockCounter = 0;
    		partCounter = 0;
    		//printAllCoeffAndR0();
    	}
    	
    	
    }
}



static void keyboard(int sig)
{
    unsigned int keys = dsp_get_keys();
    unsigned int leds = 0;

    if(keys & DSP_SW1) { // If key 1 is pressed (ON the DSP)
        leds = DSP_D1;
        //isRecording = 1;
        //printf("Start Recording\n");
        //isRecording = 1;
    } else if(keys & DSP_SW2) { // key 2 is pressed
        leds = 0;
        mute = 0;
    } else if(keys & DSP_SW3) {
        leds = DSP_D2;
    } else if(keys & DSP_SW4) {
        leds = DSP_D1|DSP_D2;
    }
    
    dsp_set_leds(leds);
}

static void timer(int sig)
{
//printf("Hello World\n");
}



static void divideIntoTen() // Divides 100x15 into 10x15 by taking the mean of 10 blocks together.
{
	int count1 = 0;
	int count2 = 0;
	int i;
	resetCoeff(); // Resets coeff to 0 to start.
	
	for (count2 = 0; count2 < numOfBlock; count2++)
	{
	
		for (i = 0; i < lambdaCoeff; i++)
		{
			coeff[count1][i] +=  allCoeff[count2][i]; // Index is one less
			
		}
	
		if ((count2+1) % 10 == 0) // Every 10 values we switch divide. 
		{
			for (i = 0; i < lambdaCoeff; i++)
			{
				coeff[count1][i] /= blockDivider;
		
			}
			count1++;	
		}
	}
	
}


void main()
{   
    /*
    Setup the DSP framework.
    */
    dsp_init(); // Call once at the beginning to initialize the framework.
    dsp_set_leds(DSP_D1 | DSP_D2); //Sets the LEDs according to a bitmask. Bit DSP_Dn (for n in ? to ?) is set in the parameter
									//if LED n is to be turned on, and reset if LED n is to be turned off
 
    /*
    Register interrupt handlers:
    SIG_SP1: the audio callback. Interrupt when processing audio data
    SIG_USR0: the keyboard callback. Interrupt when a button is pressed.
    SIG_TMZ: the timer callback. Interrupt when a timer expires. 
    */
    interrupt(SIG_SP1, process);
    interrupt(SIG_USR0, keyboard);
    interrupt(SIG_TMZ, timer);
    timer_set(9830400, 9830400);
     timer_on();

    /*
    Start the framework.
    */
    dsp_start(); //Call to start the serial ports connected to the codecs
       while(1) //static float allCoeff[numOfBlock][lambdaCoeff], allR0
    {
    	if (startProcessing == 1) // We got all our 100 blocks for 2 seconds. 
    	{
   			divideIntoTen(); // Gets our 100x15 into 10x15
   			int matchNum = matching();
   			printf("Matched: %d\n", matchNum);
   			printCoeff();
    		startProcessing = 0;
    		unsigned int leds;
    		if (matchNum == 1)
    		{
    			leds = DSP_D1;
    		}
    		else if (matchNum == 2) // number 2 lights up
    		{
    			leds = DSP_D2;
    		}
    		else if (matchNum == 3) // Both lights up
    		{
    			leds = DSP_D1|DSP_D2;
    		}
    		else // No match
    		{
    			leds = 0;
    		}
    		dsp_set_leds(leds);
    	}
    	
    	idle();
    }

  

    
    /*
    Everything is handled by the interrupt handlers, so just put an empty
    idle-loop here. If not, the program falls back to an equivalent idle-loop
    in the run-time library when main() returns.
    */
    /*for(;;) {
        idle();
    }*/
}

