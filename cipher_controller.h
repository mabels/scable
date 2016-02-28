#ifndef __scable_cipher_controller__
#define __scable_cipher_controller__

#include "rte.h"

#include "pkt_action.h"
#include "lcore_action.h"
#include "distribution_controller.h"

class CipherController {
  private:
    std::vector<PktAction> cipherActions;
    const LcoreActionDelegate<CipherController> lcoreActionDelegate;
    DistributionController &distributionController;
    struct rte_mbuf *buf = 0;

    CipherController(DistributionController &dc)
      : lcoreActionDelegate(this),
        distributionController(dc) {
    }

  public:
    static CipherController *create(DistributionController &dc) {
      auto cctl = new CipherController(dc);
      return cctl;
    }

    bool addCipherAction(const PktAction &pktAction) {
      for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
          if ((*(pktAction.getport))(pktAction.context)->getId() == (*(it->getport))(it->context)->getId()) {
            LOG(ERROR) << "redefined port for cipherAction";
            return false;
          }
      }
      cipherActions.push_back(pktAction);
      return true;
    }

    const LcoreAction& getAction() const {
      return lcoreActionDelegate.get();
    }

    const char *name() const {
      return "CipherController";
    }

    void lcorePrepare(Lcore &lcore) {
      LOG(INFO) << "Starting Lcore on:" << lcore.getId() << ":" << this << "=>"
        << name() << " with [" << join(cipherActions, ",") << "]";
      for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
        (*(it->prepare))(it->context);
      }
    }

    void lcoreAction(Lcore &lcore) {
      buf = rte_distributor_get_pkt(distributionController.getDistributor(), lcore.getId(), buf);
      rte_prefetch0(rte_pktmbuf_mtod(buf, void *));
      struct ether_hdr *eth = rte_pktmbuf_mtod(buf, struct ether_hdr *);
      for (auto it = cipherActions.begin(); it != cipherActions.end(); ++it) {
        if ((*(it->action))(it->context, buf)) {
          break;
        }
      }
    }

};

#endif
