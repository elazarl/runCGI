require 'cgi'

cgi = CGI.new

puts cgi.params.map{|name,val| name+"("+val.join(" ")+")Q"}.join("\n")
puts ENV.map{|k,v| k+"("+v+")E"}.join("\n")
