#include "market_data_publisher.h"
namespace Exchange{
    auto MarketDataPublisher::run() noexcept ->void{
        logger_.log("%:% %() %\n",
                __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_));
        while(run_){
            for(auto market_update = outgoing_md_updates_->getNextToRead(); outgoing_md_updates_->size() && market_update; market_update = outgoing_md_updates_->getNextToRead()){
                logger_.log("%:% %() % Sending seq:% %\n",
                            __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), next_inc_seq_num_, market_update->toString());
                incremental_socket_.send(&next_inc_seq_num_, sizeof(next_inc_seq_num_));
                incremental_socket_.send(market_update, sizeof(MEMarketUpdate));
                outgoing_md_updates_->updateReadIndex();
                auto next_write = snapshot_md_updates_.getNextToWriteTo();
                next_write->seq_num_ = next_inc_seq_num_;
                next_write->me_market_update_ = *market_update;
                snapshot_md_updates_.updateWriteIndex();
                ++next_inc_seq_num_;
            }
            incremental_socket_.sendAndRecv();
        }
    }
}