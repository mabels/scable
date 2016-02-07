

RTE_SDK=/root/dpdk-2.2.0
RTE_SDK=/root/dpdk
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

DEBUG=-g
PTHREAD=-pthread

UNAME_S := $(shell uname -s)
ARCH := $(UNAME_S)
MKDIR := mkdir

LDFLAGS=\
	-Wl,--no-as-needed -Wl,-export-dynamic \
	-L$(RTE_SDK)/${RTE_TARGET}/lib \
	-Wl,--whole-archive \
	-Wl,-lrte_distributor -Wl,-lrte_reorder -Wl,-lrte_kni -Wl,-lrte_pipeline \
	-Wl,-lrte_table -Wl,-lrte_port -Wl,-lrte_timer -Wl,-lrte_hash \
	-Wl,-lrte_jobstats -Wl,-lrte_lpm -Wl,-lrte_power -Wl,-lrte_acl \
	-Wl,-lrte_meter -Wl,-lrte_sched -Wl,-lm -Wl,-lrt -Wl,-lrte_vhost \
	-Wl,--start-group -Wl,-lrte_kvargs -Wl,-lrte_mbuf -Wl,-lrte_mbuf_offload \
	-Wl,-lrte_ip_frag -Wl,-lethdev -Wl,-lrte_cryptodev -Wl,-lrte_mempool \
	-Wl,-lrte_ring -Wl,-lrte_eal -Wl,-lrte_cmdline -Wl,-lrte_cfgfile \
	-Wl,-lrte_pmd_bond -Wl,-lrte_pmd_vmxnet3_uio -Wl,-lrte_pmd_virtio \
	-Wl,-lrte_pmd_cxgbe -Wl,-lrte_pmd_enic -Wl,-lrte_pmd_i40e \
	-Wl,-lrte_pmd_fm10k -Wl,-lrte_pmd_ixgbe -Wl,-lrte_pmd_e1000 \
	-Wl,-lrte_pmd_ring -Wl,-lrte_pmd_af_packet -Wl,-lrte_pmd_null -Wl,-lrt \
	-Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive

# 	$(DEBUG) -lssl -lcrypto \
# 	-lrte_distributor \
# 	-lrte_reorder \
# 	-lrte_pipeline \
# 	-lrte_table \
# 	-lrte_port \
# 	-lrte_timer \
# 	-lrte_hash \
# 	-lrte_jobstats \
# 	-lrte_lpm \
# 	-lrte_power \
# 	-lrte_acl \
# 	-lrte_meter \
# 	-lrte_sched \
# 	-lm \
# 	-lrt \
# 	-lrte_vhost \
# 	-lrte_kvargs \
# 	-lrte_mbuf \
# 	-lrte_mbuf_offload \
# 	-lrte_ip_frag \
# 	-lethdev \
# 	-lrte_cryptodev \
# 	-lrte_mempool \
# 	-lrte_ring \
# 	-lrte_eal \
# 	-lrte_cmdline \
# 	-lrte_cfgfile \
# 	-lrte_pmd_bond \
# 	-lrte_pmd_e1000 \
# 	-lrte_pmd_ixgbe \
# 	-ldl
#
#	-lrte_pmd_af_packet \
#	-lrte_kni \
#	-lethdev \
#	-lrte_pmd_cxgbe \
#	-lrte_pmd_enic \
#	-lrte_pmd_fm10k \
#	-lrte_pmd_i40e \
#	-lrte_pmd_null \
#	-lrte_pmd_ring \
#	-lrte_pmd_virtio \
#	-lrte_pmd_vmxnet3_uio
#

CXXFLAGS=$(DEBUG) $(PTHREAD) -std=c++11 -O3
OPENSSL=/usr/local/Cellar/openssl/1.0.2e_1
#ifeq ($(UNAME_S), Linux)
#	LD=g++ $(PTHREAD)
#	CXX=g++ -I$(DPDK)/include
#endif
#ifeq ($(UNAME_S),Darwin)
#	CXXFLAGS+=-Wdeprecated-declarations -I$(OPENSSL)/include
#	LD=clang++ -L$(OPENSSL)/lib
#	CXX=clang++
#endif

TEST_SOURCES=TestEnDecryption.cc
TEST_OBJECTS = $(addprefix $(ARCH)/, $(TEST_SOURCES:.cc=.o))
INTERFACER_SOURCES = scable.cc rte_controller.cc port.cc \
	rx_workers.cc rx_worker.cc \
	tx_workers.cc tx_worker.cc \
	rte.cc crypto_workers.cc \
	cipher_worker.cc decipher_worker.cc
INTERFACER_OBJECTS = $(addprefix $(ARCH)/, $(INTERFACER_SOURCES:.cc=.o))
INTERFACER_HEADERS = $(addprefix $(ARCH)/, $(INTERFACER_SOURCES:.cc=.h))
#EXES = $(OBJECTS:.o=)
#$(ARCH)/TestEnDecryption

#include $(RTE_SDK)/mk/rte.extapp.mk

all: $(ARCH) $(ARCH)/scable

$(ARCH)/TestEnDecryption: $(TEST_OBJECTS)

$(ARCH)/scable: $(INTERFACER_OBJECTS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) \
        -Wl,-Map=$(@).map,--cref -o $@ \
				$(INTERFACER_OBJECTS) $(LDFLAGS)

$(ARCH)/%.o: %.cc
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(ARCH):
	$(MKDIR) -p $(ARCH)

$(ARCH)/%: $(ARCH)/%.o
	$(LD) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(ARCH)/*
