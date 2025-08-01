#include <csignal>
#include "strategy/trade_engine.h"
#include "order_gw/order_gateway.h"
#include "market_data/market_data_consumer.h"
#include "common/logging.h"
#include "common/types.h"

using namespace thu;
Logger *logger = nullptr;
Trading::TradeEngine *trade_engine = nullptr;
Trading::MarketDataConsumer *market_data_consumer = nullptr;
Trading::OrderGateway *order_gateway = nullptr;

int main(int argc, char **argv){
    const ClientId client_id = atoi(argv[1]);
    srand(client_id);
    const auto algo_type = stringToAlgoType(argv[2]);
    TradeEngineCfgHashMap ticker_cfg;
    size_t next_ticker_id = 0;
    for(int i=3; i<argc; i+=5, ++next_ticker_id){
        ticker_cfg.at(next_ticker_id) = 
            { static_cast<Qty>(std::atoi(argv[i]))
            , std::atof(argv[i+1])
            , { static_cast<Qty>(std::atoi(argv[i+2]))
              , static_cast<Qty>(std::atoi(argv[i+3]))
              , std::atof(argv[i+4])
              }
            };
    }
    logger = new Logger("trading_main_"+std::to_string(client_id) + ".log");
    const int sleep_time = 20*1000;
    Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
    Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);
    std::string time_str;
    logger->log("%:% %() % Starting Trade Engine...\n",
                __FILE__, __LINE__, __FUNCTION__,
                thu::getCurrentTimeStr(&time_str));
    trade_engine = new Trading::TradeEngine(client_id, algo_type, ticker_cfg, &client_requests, &client_responses, &market_updates);
    trade_engine->start();

    const std::string order_gw_ip = "127.0.0.1";
    const std::string order_gw_iface = "lo";
    const int order_gw_port = 12345;
    logger->log("%:% %() % Starting Order Gateway...\n",
                __FILE__, __LINE__, __FUNCTION__,
                thu::getCurrentTimeStr(&time_str));
    order_gateway = new Trading::OrderGateway(client_id, &client_requests, &client_responses, order_gw_ip, order_gw_iface, order_gw_port);
    order_gateway->start();
    
    const std::string mkt_data_iface = "lo";
    const std::string snapshot_ip = "233.252.14.1";
    const int snapshot_port = 20000;
    const std::string incremental_ip = "233.252.14.3";
    const int incremental_port = 20001;
    logger->log("%:% %() % Starting Market Data Consumer...\n ", __FILE__, __LINE__, __FUNCTION__,
                thu::getCurrentTimeStr(&time_str));
    market_data_consumer = new Trading::MarketDataConsumer(client_id, &market_updates, mkt_data_iface, snapshot_ip, snapshot_port, incremental_ip, incremental_port);
    market_data_consumer->start();

    usleep(10*1000*1000);
    trade_engine->initLastEventTime();

    if(algo_type == AlgoType::RANDOM){
        OrderId order_id = client_id * 1000;

        std::vector<Exchange::MEClientRequest> client_requests_vec;
        std::array<Price, ME_MAX_TICKERS> ticker_base_price;
        for(size_t i=0; i<ME_MAX_TICKERS; ++i){
            ticker_base_price[i] = (rand() % 100) + 100;
        }
        trade_engine->initLastEventTime();

        for(size_t i=0; i<10000; ++i){
            const TickerId ticker_id = rand() % ME_MAX_TICKERS;
            const Price price = ticker_base_price[ticker_id] + (rand()%10) + 1;
            const Qty qty = 1 + (rand()%100) + 1;
            const Side side = (rand()%2) ? Side::BUY : Side::SELL;
            Exchange::MEClientRequest new_request{Exchange::ClientRequestType::NEW, client_id, ticker_id, order_id++, side, price, qty};
            trade_engine->sendClientRequest(&new_request);
            usleep(sleep_time);
            client_requests_vec.push_back(new_request);
            const auto cxl_index = rand()%client_requests_vec.size();
            auto cxl_request = client_requests_vec[cxl_index];
            cxl_request.type_ = Exchange::ClientRequestType::CANCEL;
            trade_engine->sendClientRequest(&cxl_request);
            usleep(sleep_time);
        }
    }

    while(trade_engine->silentSeconds() < 60){
        logger->log("%:% %() % Waiting till no activity, been silent for % seconds...\n", __FILE__, __LINE__, __FUNCTION__,
                    thu::getCurrentTimeStr(&time_str),
                    trade_engine->silentSeconds());
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(10s);
    }

    trade_engine->stop();
    market_data_consumer->stop();
    order_gateway->stop();
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10s);
    delete logger; logger=nullptr;
    delete trade_engine; trade_engine=nullptr;
    delete market_data_consumer; market_data_consumer = nullptr;
    delete order_gateway; order_gateway = nullptr;
    std::this_thread::sleep_for(10s);
    exit(EXIT_SUCCESS);
}