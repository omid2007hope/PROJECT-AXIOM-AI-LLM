FROM python:3.11-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Install Python OSINT tools
RUN pip install --no-cache-dir \
    sherlock-project \
    holehe \
    maigret \
    theharvester

# Clone and install SpiderFoot
RUN git clone https://github.com/smicallef/spiderfoot.git /opt/spiderfoot && \
    cd /opt/spiderfoot && \
    pip install --no-cache-dir -r requirements.txt

# Clone and install GHunt
RUN git clone https://github.com/mxrch/GHunt.git /opt/ghunt && \
    cd /opt/ghunt && \
    pip install --no-cache-dir -r requirements.txt

# Clone and install Blackbird
RUN git clone https://github.com/p1ngul1n0/blackbird.git /opt/blackbird && \
    cd /opt/blackbird && \
    pip install --no-cache-dir -r requirements.txt

# Copy server files
RUN mkdir -p /app/src
COPY src/osint_tools_mcp_server.py /app/src/
COPY requirements.txt /app/
RUN pip install --no-cache-dir -r requirements.txt

# Set environment variable
ENV PYTHONUNBUFFERED=1

# Run the MCP server
CMD ["python", "src/osint_tools_mcp_server.py"]