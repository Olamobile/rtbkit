/** bidding_agent_ex.cc                                 -*- C++ -*-
    Rémi Attab, 22 Feb 2013
    Copyright (c) 2013 Datacratic.  All rights reserved.
    (c) 2016 OLA Mobile, Mats Brorsson

    The first experimental bidding agent used at OLA Mobile

*/

#include "rtbkit/common/bids.h"
#include "rtbkit/core/banker/slave_banker.h"
#include "rtbkit/core/agent_configuration/agent_config.h"
#include "rtbkit/plugins/bidding_agent/bidding_agent.h"
#include "soa/service/service_utils.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
using namespace ML;

namespace RTBKIT {

/******************************************************************************/
/* Experimental OLA Mobile BIDDING AGENT                                      */
/******************************************************************************/

/** 
    Simple bidding agent whose sole purpose in life is to bid 0.1$ CPM on every
    bid requests it sees. It also has a very simple pacer which ensures that we
    always have at most 1$ to spend every 10 seconds.

 */
struct OlaBiddingAgent :
        public BiddingAgent
{
    OlaBiddingAgent(
            std::shared_ptr<Datacratic::ServiceProxies> services,
            const string& serviceName) :
        BiddingAgent(services, serviceName),
        accountSetup(false)
    {}


    void init(const std::shared_ptr<ApplicationLayer> appLayer)
    {
        // We only want to specify a subset of the callbacks so turn the
        // annoying safety belt off.
        strictMode(false);

        onBidRequest = bind(
                &OlaBiddingAgent::bid, this, _1, _2, _3, _4, _5, _6);

        // This component is used to speak with the master banker and pace the
        // rate at which we spend our budget.
        budgetController.setApplicationLayer(appLayer);
        budgetController.start();

        // Update our pacer every 10 seconds. Note that since this interacts
        // with the budgetController which is only synced up with the router
        // every few seconds, the wait period shouldn't be set too low.
        addPeriodic("OlaBiddingAgent::pace", 10.0,
                [&] (uint64_t) { this->pace(); });

        BiddingAgent::init();
    }

    void start()
    {
        BiddingAgent::start();

        // Build our configuration and tell the world about it.
        setConfig();
    }

    void shutdown()
    {
        BiddingAgent::shutdown();

        budgetController.shutdown();
    }


    /** Sets up an agent configuration for our example. */
    void setConfig()
    {
        config = AgentConfig();

        // Accounts are used to control the allocation of spending budgets for
        // an agent. The whole mechanism is fully generic and can be setup in
        // whatever you feel it bests suits you.
        config.account = {"ola_campaign_1", "fixed"};


	config.providerConfig["smaato"]["seat"] = "356";

	// A generic provider config value.
	Json::Value genericProviderConfig {};
	genericProviderConfig["smaato"]["adm"] = "http://www.mtrck.net/offer/15922%7C624?data1=Track1&data2=Track2";
	genericProviderConfig["smaato"]["nurl"] = "http://rtbtest.rtman.net:12340/wins/smaato/${AUCTION_ID}/${AUCTION_IMP_ID}/${AUCTION_PRICE}";
	

        // Specify the properties of the creatives we are trying to show.

        config.creatives.push_back(Creative(300, 250, "BigBox1", 1));
        config.creatives.push_back(Creative(216, 36, "TinyBanner", 2));
        config.creatives.push_back(Creative(300, 50, "SmallBanner", 3));
        config.creatives.push_back(Creative(120, 20, "TinierBanner", 4));
        config.creatives.push_back(Creative(320, 50, "Banner5", 5));
        config.creatives.push_back(Creative(168, 28, "TinyBanner2", 6));

	for (auto &cr : config.creatives) {

	  // Add the generic providerConfig for all creatives
	  cr.providerConfig = genericProviderConfig;

	  if (cr.name == "SmallBanner") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C618</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/300_50.gif</imgUrl><width>300</width><height>50</height></imageAd></ad>";
	  } else if (cr.name == "BigBox1") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C621</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/300_250_1.gif</imgUrl><width>300</width><height>50</height></imageAd></ad>";
	  } else if (cr.name == "TinyBanner") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C615</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/216_36.gif</imgUrl><width>216</width><height>36</height></imageAd></ad>";
	  } else if (cr.name == "TinierBanner") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C615</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/120_30.gif</imgUrl><width>120</width><height>20</height></imageAd></ad>";
	  } else if (cr.name == "Banner5") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C615</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/320_50.gif</imgUrl><width>320</width><height>50</height></imageAd></ad>";
	  } else if (cr.name == "TinyBanner2") {
	    cr.providerConfig["smaato"]["adm"] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><ad xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='smaato_ad_v0.9.xsd' modelVersion='0.9'><imageAd><clickUrl>http://www.mtrck.net/offer/15922%7C615</clickUrl><imgUrl>http://s3.eu-central-1.amazonaws.com/olacr-test/test/168_28.gif</imgUrl><width>168</width><height>28</height></imageAd></ad>";
	  }

	  cr.providerConfig["smaato"]["adid"] = cr.id;
	  cr.providerConfig["smaato"]["adomain"].append(Json::Value("mtrck.net"));
	  cr.providerConfig["smaato"]["mimeTypes"].append(Json::Value("image/gif")); 

	  // debug printouts
	  //cerr << cr.toJson() << endl;

	  //cerr << "CREATIVE Name: " << cr.name << endl;
	  //cerr << "CREATIVE width: " << cr.format.width << endl;
	  //cerr << "CREATIVE height: " << cr.format.height << endl;
	}


	// add filters as necessary


        // Indicate to the router that we want our bid requests to be augmented
        // with our frequency cap augmentor example.
	// turn it of for the Smaato compliance test.
        if (false ){
            AugmentationConfig augConfig;

            // Name of the requested augmentor.
            augConfig.name = "frequency-cap-ex";

            // If the augmentor was unable to augment our bid request then it
            // should be filtered before it makes it to our agent.
            augConfig.required = true;

            // Config parameter sent used by the augmentor to determine which
            // tag to set.
            augConfig.config = Json::Value(42);

            // Instruct to router to filter out all bid requests who have not
            // been tagged by our frequency cap augmentor.
            augConfig.filters.include.push_back("pass-frequency-cap-ex");

            config.addAugmentation(augConfig);
        }

        // Configures the agent to only receive 80% of the bid request traffic
        // that matches its filters.
	// for Smaato compliance, use 1.0
        config.bidProbability = 1.0;

        // Tell the world about our config. We can change the configuration of
        // an agent at any time by calling this function.
        doConfig(config);
    }


    /** Simple fixed price bidding strategy. Note that the router is in charge
        of making sure we stay within budget and don't go bankrupt.
    */
    void bid(
            double timestamp,
            const Id & id,
            std::shared_ptr<RTBKIT::BidRequest> br,
            Bids bids,
            double timeLeftMs,
            const Json::Value & augmentations)
    {

      //cerr << "A request for bid(s) has arrived: " << id << endl;

      //cerr << br->toJson() << endl;

        for (Bid& bid : bids) {


            // In our example, all our creatives are of the different sizes so
            // there should only ever be one biddable creative. Note that that
            // the router won't ask for bids on imp that don't have any
            // biddable creatives.
	  ExcAssertEqual(bid.availableCreatives.size(), 1);
            int availableCreative = bid.availableCreatives.front();

            // We don't really need it here but this is how you can get the
            // AdSpot and Creative object from the indexes.
            (void) br->imp[bid.spotIndex];
            (void) config.creatives[availableCreative]; 

	    double bf {br->imp[bid.spotIndex].bidfloor.val};
            Amount bidFloor = USD_CPM(bf);

	    Amount money{};
	    if (bf == 0) {
	      money = USD_CPM(1);
	    } else {
	      money = bidFloor + USD_CPM(0.1);
	    }


            bid.bid(availableCreative, money);

            // Create a 0.0001$ CPM bid with our available creative.
            // Note that by default, the bid price is set to 0 which indicates
            // that we don't wish to bid on the given spot.
            //bid.bid(availableCreative, MicroUSD(100));

	    // debug
	    //cerr << "Making a bid:" << bid.toJson() << endl;

        }

        // A value that will be passed back to us when we receive the result of
        // our bid.
        Json::Value metadata = 42;

	//cerr << "Send bid(s)" << endl;

        // Send our bid back to the agent.
        doBid(id, bids, metadata);
    }


    /** Simple pacing scheme which allocates 1$ to spend every period. */
    void pace()
    {
        // We need to register our account once with the banker service.
        if (!accountSetup) {
            accountSetup = true;
            budgetController.addAccountSync(config.account);
        }

        // Make sure we have 1$ to spend for the next period.
        budgetController.topupTransferSync(config.account, USD(1));
    }


    AgentConfig config;

    bool accountSetup;
    SlaveBudgetController budgetController;
};

} // namepsace RTBKIT


/******************************************************************************/
/* MAIN                                                                       */
/******************************************************************************/

int main(int argc, char** argv)
{
    using namespace boost::program_options;

    Datacratic::ServiceProxyArguments args;
    RTBKIT::SlaveBankerArguments bankerArgs;

    options_description options = args.makeProgramOptions();
    options.add_options()
        ("help,h", "Print this message");
    options.add(bankerArgs.makeProgramOptions());

    variables_map vm;
    store(command_line_parser(argc, argv).options(options).run(), vm);
    notify(vm);

    if (vm.count("help")) {
        cerr << options << endl;
        return 1;
    }

    auto serviceProxies = args.makeServiceProxies();
    RTBKIT::OlaBiddingAgent agent(serviceProxies, "ola-bidding-agent");
    agent.init(bankerArgs.makeApplicationLayer(serviceProxies));
    agent.start();

    while (true) this_thread::sleep_for(chrono::seconds(10));

    // Won't ever reach this point but this is how you shutdown an agent.
    agent.shutdown();

    return 0;
}

