var formidable = require('formidable');
var fs = require('fs');
var handlebars = require('handlebars');
var http = require('http');
var moment = require('moment-timezone');
var path = require('path');
var sanitizer = require('sanitizer');
var url = require('url');
var util = require('util');

var baseDirectory = __dirname + "/www/";
var port = 8000;

var server = http.createServer(function (request, response)
{
    if (request.method.toLowerCase() == 'get')
    {
        try
        {
            var requestUrl = url.parse(request.url);
     
            // need to use path.normalize so people can't access directories underneath baseDirectory
            var fsPath = baseDirectory+path.normalize(requestUrl.pathname);

            // default to index.html
            if (requestUrl.path == '/')
            {
                fsPath += "index.html";
            }
     
            response.writeHead(200);
            var fileStream = fs.createReadStream(fsPath);
            fileStream.pipe(response);
            fileStream.on('error',function(e)
            {
                response.writeHead(404);     // assume the file doesn't exist
                response.end();
            });
        }
        catch(e)
        {
            response.writeHead(500);
            response.end();     // end the response so browsers don't hang
            console.log(e.stack);
        }
    }
    else if (request.method.toLowerCase() == 'post')
    {
        fs.readFile('www/reply.html', 'utf-8', function(error, source)
        {
            var fields = [];
            var form = new formidable.IncomingForm();

            form.parse(request, function (err, fields, files)
            {
                console.log('\nRequest:')
                console.log(fields);
                
                var sys = require('sys')
                var exec = require('child_process').exec;
                var data;
                
                var sanitized_timestamp = sanitizer.escape(fields.timestamp);
                var sanitized_source_port = sanitizer.escape(fields.source_port);
                var sanitized_source_address = sanitizer.escape(fields.source_address);
    
                /**
                sanitized_timestamp = sanitized_timestamp.replace(' ', 'T');
                **/
                var utc_timestamp = moment.tz(sanitized_timestamp, 'UTC').format().substr(0,19) + "Z";

                var command = "../query/query " + utc_timestamp + " v4 " + sanitized_source_port;

                
                function puts(error, stdout, stderr)
                {
                    var data = {
                        device: stdout.split('START')[1].split('END')[0],
                    };

                    var template = handlebars.compile(source);
                    var html = template(data);
                    
                    response.writeHead(200);
                    response.write(html);
                    response.end();

                    console.log('Reply:')
                    console.log(stdout.split('START')[1].split('END')[0]);
                }

                exec(command, puts);
            });

            var sanitized_request = request;
            //var sanitized_request = sanitizer.escape(request);

            form.parse(sanitized_request);
        });
    }
});

server.listen(port);
console.log('Server listening on port ' + port);
