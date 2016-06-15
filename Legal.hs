module Legal where
import Data.Array
import Data.List
import Data.Char
import Numeric
import Data.FixedPoint

abls :: [(Int,FixedPoint10241024,FixedPoint10241024,FixedPoint10241024)]
abls = abl 1 squarish where
  abl n (nn:nn1:abl1@(n1n1:_)) = (n,a,b,l) : abl (n+1) abl1 where
    l = fromIntegral nn * fromIntegral n1n1 / (fromIntegral nn1)^2
    b = fromIntegral nn1 / (fromIntegral nn * l^n)
    a = fromIntegral nn / (b^(2*n) * l^(n^2))
  abl _ _ = []

main = mapM_ print abls
  -- print $ legal 19 19

alpha  = 0.8506399258457145
logalpha = logBase 10 alpha

beta   = 0.96553505933837387
logbeta = logBase 10 beta

lambda = 2.975734192043357249381
loglambda = logBase 10 lambda

legal m n = alpha * beta**(m+n) * lambda**(m*n)
loglegal m n = logalpha + logbeta*(m+n) + loglambda*m*n


llegal m n = (a, 10.0**b) where (a,b) = properFraction (loglegal m n)

recurrence :: [Integer] -> [Integer] -> [Integer]
recurrence coeff basis = rec where
 rec = basis ++ rest
 rest = foldl (zipWith (+)) (repeat 0) $ zipWith (map.(*)) coeff $ tails rec

fib = recurrence [1,1] [0,1]

legal1 = recurrence [1,-1,3] [1,1,5]

legal2 = recurrence [-1,2,20,-13,31,-16,10] [1,5,57,489,4125,35117,299681]

legal3 = recurrence [-5,73,100,-1402,1014,-5352,-2490,6018,-4020,1766,9083,-19993,22072,-16646,9426,-3750,1171,-233,33] [1,15,489,12675,321689,8180343,208144601,5296282323,134764135265,3429075477543,87252874774409,2220150677358587,56491766630430761,1437433832683612783,36575525011037967769,930664771769562054147,23680778803620700205625,602557764897193682879119,15332091188757329557096929]

ternary n = showIntAtBase 3 intToDigit n ""


squarish = [1,5,57,489,12675,321689,24318165,1840058693,414295148741,93332304864173,62567386502084877,41945191530093646965,83677847847984287628595,166931297609667912727898521,990966953618170260281935463385,5882748866432370655674372752123193,103919148791293834318983090438798793469,1835738613899845421140262364853644706891109,96498428501909654589630887978835098088148177857,5072588588647327658457862518216696854885169490987149,793474866816582266820936671790189132321673383112185151899,124118554774307129694783556890846966815009879092863579679259393,57774258489513238998237970307483999327287210756991189655942651331169,26892554058860272116972562366415920138007095980551558908000982332405743333,37249792307686396442294904767024517674249157948208717533254799550970595875237705,51595955665685681166597566866805181435596339502695699293823422273656970477373415200373,212667732900366224249789357650440598098805861083269127196623872213228196352455447575029701325,876571894704740435776253865561651594678577903166188258472955681125289495868953613359454403019145877,10751464308361383118768413754866123809733788820327844402764601662870883601711298309339239868998337801509491,131870512244645759297888472329947870580266256924485681728458086578687538959472921550847035733890182662513180743513,4813066963822755416429056022484299646486874100967249263944719599975607459850502222039591149331431805524655467453067042377,175669398745227675074920433327787366374551886098434473836510647159450821039563378374569811240252763776406712988379278644250456677,19079388919628199204605726181850465220151058338147922243967269231944059187214767997105992341735209230667288462179090073659712583262087437,2072205427619023303039587520236390121754274072718784609433998196933282608067036314403465202963700297341152216286750576593627459392979397487964077,669723114288829212892740188841706543509937780640178732810318337696945624428547218105214326012774371397184848890970111836283470468812827907149926502347633,216450089279078275314395453480468424469694873576469893709517750563261490751122922463339745178577954008324586419548071995019779454584564790800309660950831580481393,208168199381979984699478633344862770286522453884530548425639456820927419612738015378525648451698519643907259916015628128546089888314427129715319317557736620397247064840935,200203194086297695671447973013557850996986259152430382611235007734890620740154339541587081797890280045754305529783867873845704588723770851289942216392403148498022616435740968427261]
