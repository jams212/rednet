local rednet = require "rednet.base"

function start()
    -- DOTO ::   
    rednet.launch("netgames 192.168.180.200:12000");
    rednet.launch("depotgames"); 
end

