-c /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/benchspec/CPU2006/471.omnetpp/run/run_base_train_amd64-m64-gcc42-nn.0000 -o omnetpp.log.cmp specperl /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/bin/specdiff -m -l 10  --abstol 1e-06  --reltol 1e-05 /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/benchspec/CPU2006/471.omnetpp/data/train/output/omnetpp.log omnetpp.log
-c /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/benchspec/CPU2006/471.omnetpp/run/run_base_train_amd64-m64-gcc42-nn.0000 -o omnetpp.sca.cmp specperl /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/bin/specdiff -m -l 10  --abstol 1e-06  --reltol 1e-05 /various/SPEC-CPU-2016/SPEC_CPU2006v1.1/benchspec/CPU2006/471.omnetpp/data/train/output/omnetpp.sca omnetpp.sca
