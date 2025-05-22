import { initializeApp } from "https://www.gstatic.com/firebasejs/11.6.0/firebase-app.js";
import { getAnalytics } from "https://www.gstatic.com/firebasejs/11.6.0/firebase-analytics.js";
import { getDatabase, ref, set, onValue} from "https://www.gstatic.com/firebasejs/11.6.0/firebase-database.js";

const firebaseConfig = {
  apiKey: "AIzaSyCqVC1VxSA0q3crk_dX-tvqWWjCXDPm_TA",
  authDomain: "aldo-e1e88.firebaseapp.com",
  databaseURL: "https://aldo-e1e88-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "aldo-e1e88",
  storageBucket: "aldo-e1e88.firebasestorage.app",
  messagingSenderId: "588930251938",
  appId: "1:588930251938:web:0cddde372d1c4b06142bfb",
  measurementId: "G-BFHMXVFS1J"
};

const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);

export {app}
