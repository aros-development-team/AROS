/* ****************************************************************************
 * SOAPDataSource.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import org.openlaszlo.iv.flash.api.FlashFile;
import org.openlaszlo.iv.flash.api.Frame;
import org.openlaszlo.iv.flash.api.Script;
import org.openlaszlo.iv.flash.api.action.DoAction;
import org.openlaszlo.iv.flash.util.FlashOutput;
import org.openlaszlo.iv.flash.util.IVException;
import org.openlaszlo.iv.flash.api.action.Actions;
import org.openlaszlo.iv.flash.api.action.Program;
import org.openlaszlo.iv.flash.util.FlashBuffer;
import org.openlaszlo.server.LPS;
import org.openlaszlo.media.MimeType;
import org.openlaszlo.remote.soap.LZSOAPService;
import org.openlaszlo.remote.soap.WSDLException;
import org.openlaszlo.remote.soap.WSDLParser;
import org.openlaszlo.remote.soap.encoding.SOAPDataEncoder;
import org.openlaszlo.server.LPS;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.IOException;
import java.io.StringReader;
import java.rmi.RemoteException;
import java.util.HashMap;
import java.util.Properties;
import java.util.Vector;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.rpc.ServiceException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.axis.message.SOAPBodyElement;
import org.apache.axis.AxisFault;
import org.apache.log4j.Logger;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.w3c.dom.DOMException;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;
import org.apache.axis.AxisProperties;
import org.apache.axis.configuration.EngineConfigurationFactoryDefault;
import org.apache.axis.components.net.DefaultCommonsHTTPClientProperties;
import org.apache.axis.message.SOAPHeader;


/**
 * Data source for SOAP.
 */
public class SOAPDataSource extends DataSource
{
    private static Logger mLogger  = Logger.getLogger(SOAPDataSource.class);

    /** Static document builder. */
    static DocumentBuilder mBuilder = null;

    static String mLoadOption;

    HashMap mServiceMap = new HashMap();

    {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            mBuilder = factory.newDocumentBuilder();
        } catch (ParserConfigurationException e) {
            System.err.println("Can't create DocumentBuilder");
        }

        mLoadOption = LPS.getProperty("rpc.soap.wsdlLoadOption", "always");
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("WSDL load option is set to \"" + mLoadOption + "\"");
        }
    }

    public SOAPDataSource() {
        //------------------------------------------------------------
        // AXIS CommonsHTTPClient properties
        //------------------------------------------------------------
        String clientConfigFile = 
            LPS.getProperties().getProperty("axis.clientConfigFile", LPS.getConfigDirectory() + 
                                            File.separatorChar + "client-config.wsdd");

        AxisProperties.setProperty(EngineConfigurationFactoryDefault.OPTION_CLIENT_CONFIG_FILE, 
                                   clientConfigFile);

        // Reuse HTTPDataSource HttpClient properties.
        AxisProperties.setProperty(DefaultCommonsHTTPClientProperties.MAXIMUM_TOTAL_CONNECTIONS_PROPERTY_KEY, 
                                   "" + HTTPDataSource.getMaxTotalConnections());
        AxisProperties.setProperty(DefaultCommonsHTTPClientProperties.MAXIMUM_CONNECTIONS_PER_HOST_PROPERTY_KEY, 
                                   "" + HTTPDataSource.getMaxConnectionsPerHost());
        AxisProperties.setProperty(DefaultCommonsHTTPClientProperties.CONNECTION_POOL_TIMEOUT_KEY, 
                                   "" + HTTPDataSource.getConnectionPoolTimeout());
    }

    /**
     * @return unique name of this data source
     */
    public String name() 
    {
        return "soap";
    }

    /**
     * Sends system information to client.
     * 
     * @throws DataSourceException if there was a problem retrieving or sending
     * the data.
     */
    public Data getData(String app, HttpServletRequest req, 
                        HttpServletResponse res, long lastModifiedTime)
        throws InterruptedIOException, IOException, DataSourceException {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("getData");
        }

        String request = req.getParameter("request");
        if (request == null || request.equals(""))
            request = "invoke";

        if (request.equals("invoke")) {
            return invoke(app, req, res);
        } else if (request.equals("load")) {
            LZSOAPService service = getService(app, req, res, true);
            return new SOAPData(service.getClientSOAPService());
        }

        return exceptionToSOAPData
            ( new DataSourceException("unknown SOAP datasource request") );
    }

    public Data invoke(String app, HttpServletRequest req, 
                       HttpServletResponse res) {

        if (! req.getMethod().equals("POST")) {
            mLogger.error("request must be POST");
            return exceptionToSOAPData
                ( new DataSourceException("request must be POST") );
        }

        String xml = req.getParameter("lzpostbody");
        if (xml == null || xml.equals("")) {
            mLogger.error("no post body");
            return exceptionToSOAPData
                ( new DataSourceException("no post body") );
        }

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("lzpostbody: " + xml);
        }

        try {

            LZSOAPService service = getService(app, req, res, false);
            if (service == null) {
                return exceptionToSOAPData
                    ( new DataSourceException("could not find service") );
            }


            try {
                String operation = req.getParameter("operation");
                Object[] value = service.invoke(operation, xml);

                if ( value[0].equals("rpc") ) {

                    return programToSOAPData( (Program)value[1], (SOAPHeader)value[2] );

                } else {

                    return documentsToSOAPData((Vector)value[1], (SOAPHeader)value[2]);

                }
            } catch (AxisFault fault) {
                // Axis throws exceptions for faults
                return new SOAPData
                    ( new SOAPDataEncoder().buildFromFault(fault) );
            }

        } catch (RemoteException e) {
            mLogger.error("RemoteException: " + e.getMessage(), e);
            return exceptionToSOAPData(e);
        } catch (DataSourceException e) {
            mLogger.error("DataSourceException: " + e.getMessage(), e);
            return exceptionToSOAPData(e);
        } catch (IOException e) {
            mLogger.error("IOException: " + e.getMessage(), e);
            return exceptionToSOAPData(e);
        } catch (ServiceException e) {
            mLogger.error("ServiceException: " + e.getMessage(), e);
            return exceptionToSOAPData(e);
        } catch (Exception e) {
            mLogger.error("Exception: " + e.getMessage(), e);
            return exceptionToSOAPData(e);
        }

    }

    SOAPData exceptionToSOAPData(Exception e) {
        return new SOAPData( new SOAPDataEncoder().buildFromException(e) );
    }


    SOAPData documentsToSOAPData(Vector docs, SOAPHeader header) 
        throws DataSourceException {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("documentsToSOAPData");
        }

        if (docs.size() == 0) {
            mLogger.error("no document return");
            throw new DataSourceException("no document return"); 
        }


        try {
            return new SOAPData( new SOAPDataEncoder(docs, header) );
        } catch (Exception e) {
            mLogger.error("problems getting SOAP document element", e);
            throw new DataSourceException("problems getting SOAP document element");
        }
    }




    /**
     * Creates key for services map.
     *
     * @param wsdl WSDL URL.
     * @param service SOAP service name.
     * @param port SOAP port name.
     * @return key to use for services map.
     */
    public String key(String wsdl, String service, String port) {
        return wsdl + " " + service + " " + port;
    }


    /**
     * Get SOAP service.
     *
     * @param req
     * @param res
     * @exception DataSourceException 
     */
    synchronized public LZSOAPService getService(String app,
                                                 HttpServletRequest req, 
                                                 HttpServletResponse res, 
                                                 boolean checkLoadOption) 
        throws DataSourceException {

        String wsdl = req.getParameter("wsdl");
        String service = req.getParameter("service");
        String port = req.getParameter("port");

        if (wsdl == null || wsdl.equals("")) {
            throw new DataSourceException("no wsdl specified");
        }

        LZSOAPService soapService = null;
        if (checkLoadOption) {
            if ( "never" == mLoadOption.intern() ) {
                soapService = (LZSOAPService)mServiceMap.get( key(wsdl,service,port) );
            }
        } else {
            soapService = (LZSOAPService)mServiceMap.get( key(wsdl,service,port) );
        }
        if (soapService != null)
            return soapService;

        return fetchService(app, req, res);
    }

    /**
     * Fetch SOAP service over HTTP.
     */
    synchronized public LZSOAPService fetchService(String app, 
                                                   HttpServletRequest req,
                                                   HttpServletResponse res) 
        throws DataSourceException {

        String wsdl = req.getParameter("wsdl");
        String service = req.getParameter("service");
        String port = req.getParameter("port");

        // TODO: [2004-06-24 pkang] cache WSDLs.
        try {
            Data data;
            if (wsdl.startsWith("file:")) {
                data = FileDataSource.getFileData(app, req, res, wsdl, -1);
            } else if (wsdl.startsWith("http://")) {
                data = HTTPDataSource.getHTTPData(req, res, wsdl, -1);
            } else {
                throw new DataSourceException("unsupported WSDL protocol: " +
                                              wsdl);
            }

            String _wsdl = data.getAsString();
            LZSOAPService soapService = 
                WSDLParser.parse(wsdl, new InputSource(new StringReader(_wsdl)));

            if (soapService == null) {
                throw new DataSourceException("could not find SOAP service (" +
                                              wsdl + ", " + service + ", " + 
                                              port + ")");
            }

            mServiceMap.put( key(wsdl,soapService.getServiceName(),soapService.getPort()), 
                             soapService);

            // secondary mapping to the same service
            if (service == null || port == null) {
                mServiceMap.put( key(wsdl,service,port), soapService);
            }

            return soapService;

        } catch (IOException e) {
            mLogger.error("Exception", e);
            throw new DataSourceException(e.getMessage());
        } catch (WSDLException e) {
            mLogger.error("Exception", e);
            throw new DataSourceException(e.getMessage());
        } catch (ServiceException e) {
            mLogger.error("Exception", e);
            throw new DataSourceException(e.getMessage());
        } catch (Exception e) {
            mLogger.error("Exception", e);
            throw new DataSourceException(e.getMessage());
        }
    }


    SOAPData programToSOAPData(Program program, SOAPHeader header) 
        throws DataSourceException {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("programToSOAPData");
        }

        try {
            return new SOAPData( new SOAPDataEncoder(program, header) );
        } catch (Exception e) {
            mLogger.error("problems creating RPC program", e);
            throw new DataSourceException("problems creating RPC program");
        }
    }

    /**
     * A data object to hold session object.
     */
    public class SOAPData extends Data
    {
        byte[] mObject = null;
        SOAPDataEncoder mSOAPDataEncoder = null;
        public SOAPData(byte[] object) {
            mObject = object;
        }

        public SOAPData(SOAPDataEncoder de) {
            mSOAPDataEncoder = de;
        }

        public String getMimeType() {
            return MimeType.SWF;
        }

        public InputStream getInputStream() 
            throws IOException {
            if (mObject != null) {
                return new ByteArrayInputStream(mObject);
            } else if (mSOAPDataEncoder != null) {
                return mSOAPDataEncoder.getInputStream();
            }

            throw new IOException("no input stream");
        }

        public long size() {
            if (mObject != null) {
                return mObject.length;
            } else if (mSOAPDataEncoder != null) {
                try {
                    return mSOAPDataEncoder.getSize();
                } catch (IOException e) {
                    mLogger.warn("ioexception getting size soap data encoder");
                }
            } 

            return 0;
        }
    }
}
