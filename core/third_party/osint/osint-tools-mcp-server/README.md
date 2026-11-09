# 🔍 OSINT Tools MCP Server

[![MCP Server](https://img.shields.io/badge/MCP-Server-blue)](https://github.com/modelcontextprotocol/specification)
[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/)
[![Python](https://img.shields.io/badge/Python-3.8%2B-blue)](https://www.python.org/)
[![OSINT](https://img.shields.io/badge/OSINT-Tools-red)](https://github.com/topics/osint)

A comprehensive MCP server that exposes multiple OSINT tools to AI assistants like Claude. This server allows AI to perform sophisticated reconnaissance and information gathering tasks using industry-standard OSINT tools.

## 🛠️ Available Tools

### 🔍 **Sherlock** - Username Search
Search for usernames across 399+ social media platforms and websites. Perfect for digital footprint analysis.
- **Input**: Username
- **Output**: List of platforms where username exists

### 📧 **Holehe** - Email Verification  
Check if an email is registered on 120+ platforms. Lightning fast and accurate.
- **Input**: Email address
- **Output**: Platforms where email is registered

### 🕷️ **SpiderFoot** - Comprehensive OSINT
The Swiss Army knife of OSINT. Performs deep reconnaissance with automatic target type detection.
- **Input**: IP, domain, email, phone, username, person name, Bitcoin address, or network block
- **Output**: Comprehensive intelligence report
- **⚠️ Note**: SpiderFoot can take 5-30 minutes to complete a full scan. Be patient!

### 🔎 **GHunt** - Google Account Intel
Extract information from Google accounts using email or Google ID.
- **Input**: Email or Google ID
- **Output**: Google account details and associated information

### 🌐 **Maigret** - Advanced Username Search
Search across 3000+ sites with false positive detection and detailed analysis.
- **Input**: Username
- **Output**: Detailed report with confidence scores

### 🌾 **TheHarvester** - Domain Intelligence
Gather emails, subdomains, hosts, employee names, and more from public sources.
- **Input**: Domain or company name
- **Output**: Comprehensive domain intelligence

### 🐦 **Blackbird** - Fast Username OSINT
Lightning-fast searches across 581 sites for username reconnaissance.
- **Input**: Username
- **Output**: Quick profile discovery results

## 🚀 Installation

### MCP Server Setup

1. **Clone this repository:**
```bash
git clone https://github.com/frishtik/osint-tools-mcp-server.git
cd osint-tools-mcp-server
```

2. **Install dependencies:**
```bash
pip install -r requirements.txt
```
This will automatically install Sherlock, Holehe, Maigret, and TheHarvester.

3. **Configure Claude Desktop:**

Add to your Claude Desktop configuration (`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS):

```json
{
  "mcpServers": {
    "osint-tools": {
      "command": "python",
      "args": ["/path/to/osint-tools-mcp-server/src/osint_tools_mcp_server.py"],
      "env": {
        "PYTHONUNBUFFERED": "1"
      }
    }
  }
}
```

4. **Restart Claude Desktop** to load the new MCP server.

### Prerequisites for Additional Tools

Some tools require manual installation due to their complexity:

#### SpiderFoot
```bash
git clone https://github.com/smicallef/spiderfoot.git /opt/spiderfoot
cd /opt/spiderfoot
pip install -r requirements.txt
```

#### GHunt
```bash
git clone https://github.com/mxrch/GHunt.git /opt/ghunt
cd /opt/ghunt
pip install -r requirements.txt
```

#### Blackbird
```bash
git clone https://github.com/p1ngul1n0/blackbird.git /opt/blackbird
cd /opt/blackbird
pip install -r requirements.txt
```

## 🎮 Usage Tips

### Getting Started

Working with AI for OSINT is a bit of an art form. Here's how to get the best results:

#### Start Simple with Holehe
I recommend starting with the **Holehe** tool - it's fast, reliable, and gives you immediate results:

```
"Check if john.doe@example.com is registered on any platforms"
```

#### Level Up to Username Searches
Once you're comfortable, try username searches with Sherlock or Maigret:

```
"Search for the username 'johndoe123' across social media platforms"
```

#### Complex Orchestrations
Here's where it gets interesting. You can chain tools together:

```
"I found an email address contact@suspicious-site.com. Can you:
1. Check what platforms it's registered on
2. Extract the domain and search for subdomains and other emails
3. Search for any usernames associated with this domain"
```

#### Let the AI Be Smart
Sometimes the best approach is to give Claude context and let it decide:

```
"I'm investigating the digital footprint of username 'hackerman2024'. 
Use your judgment to gather as much information as possible."
```

### Pro Tips 🎯

1. **Be Patient with SpiderFoot**: It's incredibly thorough but can take up to 30 minutes for a full scan. Start it and grab a coffee!

2. **Parallel Processing**: Claude can run multiple tools simultaneously. Don't hesitate to ask for parallel searches:
   ```
   "Search for 'johndoe' on both Sherlock and Maigret at the same time"
   ```

3. **Know When to Hold the Leash**: 
   - For specific investigations: Be explicit about which tools to use
   - For exploratory research: Let Claude choose the tools
   - For time-sensitive tasks: Avoid SpiderFoot, stick to faster tools

4. **Cross-Reference Results**: Different tools have different databases. Maigret might find accounts that Sherlock misses and vice versa.

5. **Email First, Username Second**: If you have an email, start there - it's usually more unique than usernames.

## ⚖️ Ethical Usage & Legal Compliance

**🚨 IMPORTANT: This tool is for legitimate security research and OSINT investigations only.**

### You MUST:
- ✅ Only gather publicly available information
- ✅ Respect privacy laws in your jurisdiction (GDPR, CCPA, etc.)
- ✅ Follow platforms' Terms of Service
- ✅ Use findings responsibly and ethically
- ✅ Obtain proper authorization for any professional investigations

### You MUST NOT:
- ❌ Use this for stalking, harassment, or any malicious purpose
- ❌ Violate any local, state, or federal laws
- ❌ Access private or protected information
- ❌ Use findings to harm individuals or organizations

## 🔧 Troubleshooting

### Common Issues

**Tools not found**: Make sure all OSINT tools are installed and in your PATH:
```bash
which sherlock holehe maigret theharvester
```

**SpiderFoot errors**: Ensure SpiderFoot is installed in `/opt/spiderfoot` or update the path in the code.

**Timeout issues**: Some tools may timeout on slow connections. Try increasing the timeout parameter:
```
"Search for username with a 30 second timeout"
```

**Rate limiting**: Some platforms rate-limit searches. If you're getting blocked, wait a bit and try again.

## 🏗️ Architecture

This MCP server uses Python's asyncio for non-blocking tool execution. Each tool runs in a subprocess, allowing for parallel execution and proper timeout handling.

```
Claude Desktop <-> MCP Protocol <-> OSINT MCP Server <-> OSINT Tools
```

## 🤝 Contributing

Found a bug? Want to add a new tool? Contributions are welcome! 

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/NewOSINTTool`)
3. Commit your changes (`git commit -m 'Add NewOSINTTool support'`)
4. Push to the branch (`git push origin feature/NewOSINTTool`)
5. Open a Pull Request

## 📚 Acknowledgments

Special thanks to these awesome projects:
- [mcp-maigret](https://github.com/BurtTheCoder/mcp-maigret) - Inspiration for MCP implementation and README structure. Go give them a ⭐!
- [Model Context Protocol](https://github.com/modelcontextprotocol/specification) - The protocol making all this possible
- All the incredible OSINT tool maintainers

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Disclaimer

This tool is provided for educational and legitimate security research purposes only. The authors are not responsible for any misuse or damage caused by this program. Use at your own risk and always ensure you have proper authorization before conducting any investigations.

---

**Remember**: With great power comes great responsibility. Use these tools wisely and ethically! 🦸‍♂️

Built with ❤️ for the OSINT community
