import http.server
import socketserver
import mimetypes
import os
import sys

# Define the port
PORT = 8000

# Ensure we are serving the correct directory (the directory where this script is)
web_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(web_dir)

# Force correct MIME type for JavaScript
# Windows registry sometimes messes this up, setting it to text/plain
mimetypes.add_type('application/javascript', '.js')
mimetypes.add_type('text/css', '.css')
mimetypes.add_type('text/html', '.html')

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers just in case, though not strictly needed for localhost-to-localhost source
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()
    
    # Override extensions_map to be sure
    extensions_map = http.server.SimpleHTTPRequestHandler.extensions_map.copy()
    extensions_map.update({
        '': 'application/octet-stream',
        '.js': 'application/javascript',
        '.css': 'text/css',
        '.html': 'text/html',
        '.png': 'image/png',
        '.jpg': 'image/jpeg',
    })

print(f"Starting server at http://localhost:{PORT}")
print(f"Serving directory: {web_dir}")
print("MIME types configured.")

try:
    with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
        print("Server running... Press Ctrl+C to stop.")
        httpd.serve_forever()
except OSError as e:
    if e.errno == 98 or e.errno == 10048: # Address already in use
        print(f"Port {PORT} is busy. Trying {PORT+1}...")
        with socketserver.TCPServer(("", PORT+1), CustomHandler) as httpd:
            print(f"Server running at http://localhost:{PORT+1}")
            httpd.serve_forever()
    else:
        raise e
except KeyboardInterrupt:
    print("\nServer stopped.")
