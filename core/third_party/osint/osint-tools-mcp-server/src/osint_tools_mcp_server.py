#!/usr/bin/env python3
"""
OSINT Tools MCP Server
A simple MCP server that exposes OSINT tools through stdio interface.
"""

import asyncio
import json
import subprocess
import tempfile
import os
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

async def run_command_in_venv(command: List[str], cwd: Optional[str] = None, input_data: Optional[str] = None) -> tuple[str, str, int]:
    """Run a command in the virtual environment."""
    try:
        # Set up environment - use system Python in container
        env = os.environ.copy()
        
        process = await asyncio.create_subprocess_exec(
            *command,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            cwd=cwd,
            env=env,
            stdin=asyncio.subprocess.PIPE if input_data else None
        )
        
        stdout, stderr = await process.communicate(input=input_data.encode() if input_data else None)
        
        return stdout.decode('utf-8', errors='ignore'), stderr.decode('utf-8', errors='ignore'), process.returncode
        
    except Exception as e:
        return "", str(e), 1

async def handle_sherlock(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle Sherlock username search."""
    username = params["username"]
    timeout = params.get("timeout", 10000)
    sites = params.get("sites", [])
    output_format = params.get("output_format", "csv")
    
    cmd = ["sherlock", username, f"--timeout", str(timeout)]
    
    if sites:
        for site in sites:
            cmd.extend(["--site", site])
            
    if output_format == "csv":
        cmd.append("--csv")
    elif output_format == "xlsx":
        cmd.append("--xlsx")
        
    # Create temporary directory for output
    with tempfile.TemporaryDirectory() as temp_dir:
        cmd.extend(["--folderoutput", temp_dir])
        
        stdout, stderr, returncode = await run_command_in_venv(cmd)
        
        if returncode == 0:
            # Read output files
            output_files = list(Path(temp_dir).glob(f"{username}.*"))
            results = {"stdout": stdout, "files": []}
            
            for file_path in output_files:
                try:
                    content = file_path.read_text(encoding='utf-8')
                    results["files"].append({
                        "filename": file_path.name,
                        "content": content
                    })
                except Exception as e:
                    print(f"Could not read file {file_path}: {e}", file=sys.stderr)
            
            return {"success": True, "content": results}
        else:
            return {"success": False, "error": f"Sherlock failed: {stderr}"}

async def handle_holehe(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle Holehe email search."""
    email = params["email"]
    only_used = params.get("only_used", True)
    timeout = params.get("timeout", 10000)
    
    cmd = ["holehe", email, "--timeout", str(timeout)]
    if only_used:
        cmd.append("--only-used")
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"Holehe failed: {stderr}"}

async def handle_spiderfoot(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle SpiderFoot comprehensive OSINT scan."""
    target = params["target"]
    
    cmd = ["python3", "/opt/spiderfoot/sf.py", 
           "-s", target,
           "-u", "all",      # Use all modules (gracefully skips those needing APIs)
           "-o", "json",     # JSON output
           "-q"]             # Quiet mode
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"SpiderFoot failed: {stderr}"}

async def handle_ghunt(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle GHunt Google account search."""
    identifier = params["identifier"]
    timeout = params.get("timeout", 10000)
    
    cmd = ["ghunt", "email", identifier]
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"GHunt failed: {stderr}"}

async def handle_maigret(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle Maigret username search."""
    username = params["username"]
    timeout = params.get("timeout", 10000)
    
    cmd = ["maigret", username, "--timeout", str(timeout), "--json"]
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"Maigret failed: {stderr}"}

async def handle_theharvester(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle theHarvester domain/email enumeration."""
    domain = params["domain"]
    sources = params.get("sources", "all")
    limit = params.get("limit", 500)
    
    cmd = ["theHarvester", "-d", domain, "-b", sources, "-l", str(limit)]
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"theHarvester failed: {stderr}"}

async def handle_blackbird(params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle Blackbird username search."""
    username = params["username"]
    timeout = params.get("timeout", 10000)
    
    cmd = ["python3", "/opt/blackbird/blackbird.py", "-u", username, "--timeout", str(timeout)]
    
    stdout, stderr, returncode = await run_command_in_venv(cmd)
    
    if returncode == 0:
        return {"success": True, "content": stdout}
    else:
        return {"success": False, "error": f"Blackbird failed: {stderr}"}

async def handle_tool_call(tool_name: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Handle tool calls by routing to appropriate handlers."""
    try:
        if tool_name == "sherlock_username_search":
            return await handle_sherlock(params)
        elif tool_name == "holehe_email_search":
            return await handle_holehe(params)
        elif tool_name == "spiderfoot_scan":
            return await handle_spiderfoot(params)
        elif tool_name == "ghunt_google_search":
            return await handle_ghunt(params)
        elif tool_name == "maigret_username_search":
            return await handle_maigret(params)
        elif tool_name == "theharvester_domain_search":
            return await handle_theharvester(params)
        elif tool_name == "blackbird_username_search":
            return await handle_blackbird(params)
        else:
            return {"success": False, "error": f"Unknown tool: {tool_name}"}
    except Exception as e:
        return {"success": False, "error": f"Tool execution failed: {str(e)}"}

async def main():
    """Main MCP server loop - handles JSON-RPC over stdio."""
    try:
        # Read from stdin and write to stdout
        while True:
            try:
                line = await asyncio.get_event_loop().run_in_executor(None, sys.stdin.readline)
                if not line:
                    break
                
                # Parse JSON-RPC request
                request = json.loads(line.strip())
                
                # Extract method and params
                method = request.get("method")
                params = request.get("params", {})
                request_id = request.get("id")
                
                # Handle different MCP methods
                if method == "initialize":
                    response = {
                        "jsonrpc": "2.0",
                        "id": request_id,
                        "result": {
                            "protocolVersion": "2024-11-05",
                            "capabilities": {
                                "tools": {}
                            },
                            "serverInfo": {
                                "name": "osint-tools-mcp-server",
                                "version": "1.0.0"
                            }
                        }
                    }
                elif method == "tools/list":
                    tools = [
                        {
                            "name": "sherlock_username_search",
                            "description": "Search for username across 399+ social media platforms and websites",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "username": {"type": "string", "description": "Username to search for"},
                                    "timeout": {"type": "integer", "description": "Timeout in seconds (default: 10000)"},
                                    "sites": {"type": "array", "items": {"type": "string"}, "description": "Specific sites to search"},
                                    "output_format": {"type": "string", "enum": ["txt", "csv", "xlsx"], "description": "Output format"}
                                },
                                "required": ["username"]
                            }
                        },
                        {
                            "name": "holehe_email_search", 
                            "description": "Check if email is registered on 120+ platforms",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "email": {"type": "string", "description": "Email address to investigate"},
                                    "only_used": {"type": "boolean", "description": "Show only registered accounts (default: true)"},
                                    "timeout": {"type": "integer", "description": "Request timeout in seconds (default: 10000)"}
                                },
                                "required": ["email"]
                            }
                        },
                        {
                            "name": "spiderfoot_scan",
                            "description": "Comprehensive OSINT scan - auto-detects target type (IP, IPv6, domain, email, phone, username, person name, Bitcoin address, network block, BGP AS)",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "target": {
                                        "type": "string", 
                                        "description": "Target to scan - SpiderFoot auto-detects type from: IP address, IPv6 address, domain, email, phone number, username, person name, Bitcoin address, network block, or BGP AS"
                                    }
                                },
                                "required": ["target"]
                            }
                        },
                        {
                            "name": "ghunt_google_search",
                            "description": "Search for Google account information using email address or Google ID",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "identifier": {"type": "string", "description": "Email address or Google ID to search"},
                                    "timeout": {"type": "integer", "description": "Timeout in seconds (default: 10000)"}
                                },
                                "required": ["identifier"]
                            }
                        },
                        {
                            "name": "maigret_username_search",
                            "description": "Search for username across 3000+ sites with detailed analysis and false positive detection",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "username": {"type": "string", "description": "Username to search for"},
                                    "timeout": {"type": "integer", "description": "Timeout in seconds (default: 10000)"}
                                },
                                "required": ["username"]
                            }
                        },
                        {
                            "name": "theharvester_domain_search",
                            "description": "Gather emails, subdomains, hosts, employee names, open ports and banners from public sources",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "domain": {"type": "string", "description": "Domain/company name to search"},
                                    "sources": {"type": "string", "description": "Data sources (default: all). Options: baidu, bing, bingapi, certspotter, crtsh, dnsdumpster, duckduckgo, github-code, google, hackertarget, hunter, linkedin, linkedin_links, otx, pentesttools, projectdiscovery, qwant, rapiddns, securityTrails, sublist3r, threatcrowd, threatminer, trello, twitter, urlscan, virustotal, yahoo"},
                                    "limit": {"type": "integer", "description": "Limit results (default: 500)"}
                                },
                                "required": ["domain"]
                            }
                        },
                        {
                            "name": "blackbird_username_search",
                            "description": "Fast OSINT tool to search for accounts by username across 581 sites",
                            "inputSchema": {
                                "type": "object",
                                "properties": {
                                    "username": {"type": "string", "description": "Username to search for"},
                                    "timeout": {"type": "integer", "description": "Timeout in seconds (default: 10000)"}
                                },
                                "required": ["username"]
                            }
                        }
                    ]
                    response = {
                        "jsonrpc": "2.0",
                        "id": request_id,
                        "result": {"tools": tools}
                    }
                elif method == "tools/call":
                    tool_name = params.get("name")
                    tool_params = params.get("arguments", {})
                    
                    result = await handle_tool_call(tool_name, tool_params)
                    
                    response = {
                        "jsonrpc": "2.0",
                        "id": request_id,
                        "result": {
                            "content": [
                                {
                                    "type": "text",
                                    "text": json.dumps(result, indent=2)
                                }
                            ]
                        }
                    }
                else:
                    response = {
                        "jsonrpc": "2.0",
                        "id": request_id,
                        "error": {
                            "code": -32601,
                            "message": f"Method not found: {method}"
                        }
                    }
                
                # Send response
                print(json.dumps(response), flush=True)
                
            except json.JSONDecodeError as e:
                error_response = {
                    "jsonrpc": "2.0",
                    "id": None,
                    "error": {
                        "code": -32700,
                        "message": f"Parse error: {str(e)}"
                    }
                }
                print(json.dumps(error_response), flush=True)
            except Exception as e:
                error_response = {
                    "jsonrpc": "2.0", 
                    "id": request.get("id") if 'request' in locals() else None,
                    "error": {
                        "code": -32603,
                        "message": f"Internal error: {str(e)}"
                    }
                }
                print(json.dumps(error_response), flush=True)
                
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Server error: {e}", file=sys.stderr)

if __name__ == "__main__":
    asyncio.run(main())